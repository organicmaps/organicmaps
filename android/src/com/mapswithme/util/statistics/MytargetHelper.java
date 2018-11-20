package com.mapswithme.util.statistics;

import android.app.Activity;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.annotation.WorkerThread;

import com.mapswithme.maps.PrivateVariables;
import com.mapswithme.util.ConnectionState;
import com.mapswithme.util.concurrency.ThreadPool;
import com.mapswithme.util.concurrency.UiThread;
import com.my.target.nativeads.NativeAppwallAd;
import com.my.target.nativeads.banners.NativeAppwallBanner;

import java.io.IOException;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.List;

import static com.mapswithme.maps.MwmApplication.prefs;

public final class MytargetHelper
{
  // for caching of myTarget setting achieved from server
  private static final String PREF_CHECK = "MyTargetCheck";
  private static final String PREF_CHECK_MILLIS = "MyTargetCheckTimestamp";
  private static final String CHECK_URL = PrivateVariables.myTargetCheckUrl();
  private static final long CHECK_INTERVAL_MILLIS = PrivateVariables.myTargetCheckInterval() * 1000;

  private static final int TIMEOUT = 1000;

  @Nullable
  private NativeAppwallAd mShowcase;
  private boolean mCancelled;

  public interface Listener<T>
  {
    void onNoAds();
    void onDataReady(@Nullable T data);
  }

  public MytargetHelper(final @NonNull Listener<Void> listener)
  {
    if (!ConnectionState.isConnected())
    {
      listener.onNoAds();
      return;
    }

    ThreadPool.getWorker().execute(new Runnable()
    {
      @Override
      public void run()
      {
        final boolean showShowcase = getShowcaseSetting();

        if (mCancelled)
          return;

        UiThread.run(new Runnable()
        {
          @Override
          public void run()
          {
            if (mCancelled)
              return;

            if (showShowcase)
              listener.onDataReady(null);
            else
              listener.onNoAds();
          }
        });
      }
    });
  }

  public void cancel()
  {
    mCancelled = true;
  }

  @WorkerThread
  private static boolean getShowcaseSetting()
  {
    final long lastCheckMillis = prefs().getLong(PREF_CHECK_MILLIS, 0);
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

  public void loadShowcase(final @NonNull Listener<List<NativeAppwallBanner>> listener, Activity activity)
  {
    if (mShowcase == null)
      mShowcase = loadAds(listener, activity);
  }

  public void handleBannersShow(@NonNull List<NativeAppwallBanner> banners)
  {
    if (mShowcase != null)
      mShowcase.handleBannersShow(banners);
  }

  private NativeAppwallAd loadAds(final @NonNull Listener<List<NativeAppwallBanner>> listener, Activity activity)
  {
    NativeAppwallAd res = new NativeAppwallAd(PrivateVariables.myTargetSlot(), activity);
    res.setListener(new NativeAppwallAd.AppwallAdListener()
    {
      @Override
      public void onLoad(NativeAppwallAd ad)
      {
        if (mCancelled)
          return;

        if (ad.getBanners().isEmpty())
          listener.onNoAds();
        else
          listener.onDataReady(ad.getBanners());
      }

      @Override
      public void onNoAd(String s, NativeAppwallAd nativeAppwallAd)
      {
        listener.onNoAds();
      }

      @Override
      public void onClick(NativeAppwallBanner nativeAppwallBanner, NativeAppwallAd nativeAppwallAd) {}

      @Override
      public void onDisplay(@NonNull NativeAppwallAd nativeAppwallAd)
      {
        /* Do nothing */
      }

      @Override
      public void onDismiss(@NonNull NativeAppwallAd nativeAppwallAd)
      {
        /* Do nothing */
      }
    });

    res.load();
    return res;
  }

  public void onBannerClick(@NonNull NativeAppwallBanner banner)
  {
    if (mShowcase != null)
      mShowcase.handleBannerClick(banner);
  }

  public static boolean isShowcaseSwitchedOnServer()
  {
    return prefs().getBoolean(PREF_CHECK, true);
  }

  private static void setShowcaseSwitchedOnServer(boolean switchedOn)
  {
    prefs().edit()
           .putLong(PREF_CHECK_MILLIS, System.currentTimeMillis())
           .putBoolean(PREF_CHECK, switchedOn)
           .apply();
  }
}
