package com.mapswithme.maps.base;

import android.content.Intent;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import android.text.TextUtils;
import android.util.Log;

import com.mapswithme.util.Config;
import com.mapswithme.util.CrashlyticsUtils;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.ViewServer;
import com.mapswithme.util.concurrency.UiThread;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;
import com.mapswithme.util.statistics.Statistics;

public class BaseActivityDelegate
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
  private static final String TAG = BaseActivityDelegate.class.getSimpleName();
  @NonNull
  private final BaseActivity mActivity;
  @Nullable
  private String mThemeName;

  public BaseActivityDelegate(@NonNull BaseActivity activity)
  {
    mActivity = activity;
  }

  public void onNewIntent(@NonNull Intent intent)
  {
    logLifecycleMethod("onNewIntent(" + intent + ")");
  }

  public void onCreate()
  {
    logLifecycleMethod("onCreate()");
    mThemeName = Config.getCurrentUiTheme(mActivity.get().getApplicationContext());
    if (!TextUtils.isEmpty(mThemeName))
      mActivity.get().setTheme(mActivity.getThemeResourceId(mThemeName));
  }

  public void onSafeCreate()
  {
    logLifecycleMethod("onSafeCreate()");
  }

  public void onSafeDestroy()
  {
    logLifecycleMethod("onSafeDestroy()");
  }

  public void onDestroy()
  {
    logLifecycleMethod("onDestroy()");
    ViewServer.get(mActivity.get()).removeWindow(mActivity.get());
  }

  public void onPostCreate()
  {
    logLifecycleMethod("onPostCreate()");
    ViewServer.get(mActivity.get()).addWindow(mActivity.get());
  }

  public void onStart()
  {
    logLifecycleMethod("onStart()");
    Statistics.INSTANCE.startActivity(mActivity.get());
  }

  public void onStop()
  {
    logLifecycleMethod("onStop()");
    Statistics.INSTANCE.stopActivity(mActivity.get());
  }

  public void onResume()
  {
    logLifecycleMethod("onResume()");
    org.alohalytics.Statistics.logEvent("$onResume", mActivity.getClass().getSimpleName() + ":" +
                                                     UiUtils.deviceOrientationAsString(mActivity.get()));
    ViewServer.get(mActivity.get()).setFocusedWindow(mActivity.get());
  }

  public void onPause()
  {
    logLifecycleMethod("onPause()");
    org.alohalytics.Statistics.logEvent("$onPause", mActivity.getClass().getSimpleName());
  }

  public void onPostResume()
  {
    logLifecycleMethod("onPostResume()");
    if (!TextUtils.isEmpty(mThemeName) &&
        mThemeName.equals(Config.getCurrentUiTheme(mActivity.get().getApplicationContext())))
      return;

    // Workaround described in https://code.google.com/p/android/issues/detail?id=93731
    UiThread.runLater(() -> mActivity.get().recreate());
  }

  private void logLifecycleMethod(@NonNull String method)
  {
    String msg = mActivity.getClass().getSimpleName() + ": " + method + " activity: " + mActivity;
    CrashlyticsUtils.INSTANCE.log(Log.INFO, TAG, msg);
    LOGGER.i(TAG, msg);
  }
}
