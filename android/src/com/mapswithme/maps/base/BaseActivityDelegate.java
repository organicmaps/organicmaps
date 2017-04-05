package com.mapswithme.maps.base;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.text.TextUtils;

import com.mapswithme.util.Config;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.ViewServer;
import com.mapswithme.util.concurrency.UiThread;
import com.mapswithme.util.statistics.Statistics;
import com.my.tracker.MyTracker;

class BaseActivityDelegate
{
  @NonNull
  private final BaseActivity mActivity;
  @Nullable
  private String mThemeName;

  BaseActivityDelegate(@NonNull BaseActivity activity)
  {
    mActivity = activity;
  }

  public void onCreate()
  {
    mThemeName = Config.getCurrentUiTheme();
    if (!TextUtils.isEmpty(mThemeName))
      mActivity.get().setTheme(mActivity.getThemeResourceId(mThemeName));
  }

  void onDestroy()
  {
    ViewServer.get(mActivity.get()).removeWindow(mActivity.get());
  }

  void onPostCreate()
  {
    ViewServer.get(mActivity.get()).addWindow(mActivity.get());
  }

  void onStart()
  {
    Statistics.INSTANCE.startActivity(mActivity.get());
    MyTracker.onStartActivity(mActivity.get());
  }

  void onStop()
  {
    Statistics.INSTANCE.stopActivity(mActivity.get());
    MyTracker.onStopActivity(mActivity.get());
  }

  public void onResume()
  {
    org.alohalytics.Statistics.logEvent("$onResume", mActivity.getClass().getSimpleName() + ":" +
                                                     UiUtils.deviceOrientationAsString(mActivity.get()));
    ViewServer.get(mActivity.get()).setFocusedWindow(mActivity.get());
  }

  public void onPause()
  {
    org.alohalytics.Statistics.logEvent("$onPause", mActivity.getClass().getSimpleName());
  }

  void onPostResume()
  {
    if (!TextUtils.isEmpty(mThemeName) && mThemeName.equals(Config.getCurrentUiTheme()))
      return;

    // Workaround described in https://code.google.com/p/android/issues/detail?id=93731
    UiThread.runLater(new Runnable() {
      @Override
      public void run() {
        mActivity.get().recreate();
      }
    });
  }
}
