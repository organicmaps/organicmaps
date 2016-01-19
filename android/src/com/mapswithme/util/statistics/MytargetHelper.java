package com.mapswithme.util.statistics;

import android.app.Activity;
import android.preference.PreferenceManager;
import android.support.annotation.NonNull;
import android.support.annotation.WorkerThread;

import java.io.IOException;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.PrivateVariables;
import com.mapswithme.maps.R;
import com.mapswithme.util.ConnectionState;
import com.mapswithme.util.concurrency.ThreadPool;
import com.mapswithme.util.concurrency.UiThread;
import ru.mail.android.mytarget.core.net.Hosts;
import ru.mail.android.mytarget.nativeads.NativeAppwallAd;

public final class MytargetHelper
{
  // for caching of myTarget setting achieved from server
  private static final String PREF_CHECK = "MyTargetCheck";
  private static final String PREF_CHECK_MILLIS = "MyTargetCheckTimestamp";
  private static final String CHECK_URL = PrivateVariables.myTargetCheckUrl();
  private static final long CHECK_INTERVAL_MILLIS = PrivateVariables.myTargetCheckInterval() * 1000;

  private static final int TIMEOUT = 1000;

  private NativeAppwallAd mShowcase;
  private Activity mActivity;
  private NativeAppwallAd.AppwallAdListener mListener;

  static
  {
    Hosts.setMyComHost();
  }

  public MytargetHelper(@NonNull NativeAppwallAd.AppwallAdListener listener, @NonNull Activity activity)
  {
    mListener = listener;
    mActivity = activity;

    if (!ConnectionState.isConnected() ||
        !isShowcaseSwitchedOnLocal())
    {
      listener.onNoAd("Switched off", null);
      return;
    }

    ThreadPool.getWorker().execute(new Runnable()
    {
      @Override
      public void run()
      {
        final boolean showShowcase = getShowcaseSetting();
        UiThread.run(new Runnable()
        {
          @Override
          public void run()
          {
            if (mListener == null || mActivity == null)
              return;

            if (!showShowcase)
            {
              mListener.onNoAd("Switched off on server", null);
              return;
            }

            loadShowcase(mListener, mActivity);
          }
        });
      }
    });
  }

  public void cancel()
  {
    mListener = null;
    mActivity = null;
  }

  @WorkerThread
  private static boolean getShowcaseSetting()
  {
    final long lastCheckMillis = MwmApplication.prefs().getLong(PREF_CHECK_MILLIS, 0);
    final long currentMillis = System.currentTimeMillis();
    if (currentMillis - lastCheckMillis < CHECK_INTERVAL_MILLIS)
      return isShowcaseSwitchedOnServer();

    HttpURLConnection connection = null;
    try
    {
      final URL url = new URL(CHECK_URL);
      connection = (HttpURLConnection) url.openConnection();
      connection.setRequestMethod("HEAD");
      // bugfix for HEAD requests on pre-JB devices https://code.google.com/p/android/issues/detail?id=24672
      connection.setRequestProperty("Accept-Encoding", "");
      connection.setConnectTimeout(TIMEOUT);
      connection.setReadTimeout(TIMEOUT);
      connection.connect();

      final boolean showShowcase = connection.getResponseCode() == HttpURLConnection.HTTP_OK;
      setShowcaseSwitchedOnServer(showShowcase);

      return showShowcase;
    } catch (MalformedURLException ignored)
    {
    } catch (IOException e)
    {
      e.printStackTrace();
    } finally
    {
      if (connection != null)
        connection.disconnect();
    }

    return false;
  }

  private void loadShowcase(NativeAppwallAd.AppwallAdListener listener, Activity activity)
  {
    mShowcase = new NativeAppwallAd(Integer.parseInt(PrivateVariables.myTargetSlot()), activity);
    mShowcase.setListener(listener);
    mShowcase.load();
  }

  public void displayShowcase()
  {
    mShowcase.show();
  }

  public static boolean isShowcaseSwitchedOnLocal()
  {
    return PreferenceManager.getDefaultSharedPreferences(MwmApplication.get())
                             .getBoolean(MwmApplication.get().getString(R.string.pref_showcase_switched_on), false);
  }

  public static boolean isShowcaseSwitchedOnServer()
  {
    return MwmApplication.prefs().getBoolean(PREF_CHECK, false);
  }

  private static void setShowcaseSwitchedOnServer(boolean switchedOn)
  {
    MwmApplication.prefs().edit().putLong(PREF_CHECK_MILLIS, System.currentTimeMillis())
                                 .putBoolean(PREF_CHECK, switchedOn).apply();
  }
}
