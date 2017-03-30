package com.mapswithme.maps.base;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.mapswithme.util.Config;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.ViewServer;
import com.mapswithme.util.concurrency.UiThread;
import com.mapswithme.util.statistics.Statistics;
import com.my.tracker.MyTracker;

public class BaseActivityDelegate
{
  @NonNull
  private final BaseActivity mActivity;
  @Nullable
  private String mThemeName;

  public BaseActivityDelegate(@NonNull BaseActivity activity)
  {
    mActivity = activity;
  }

  public void onCreate()
  {
    mThemeName = Config.getCurrentUiTheme();
    if (mThemeName != null)
      mActivity.get().setTheme(mActivity.getThemeResourceId(mThemeName));
  }

  public void onDestroy()
  {
    ViewServer.get(mActivity.get()).removeWindow(mActivity.get());
  }

  public void onPostCreate()
  {
    ViewServer.get(mActivity.get()).addWindow(mActivity.get());
  }

  public void onStart()
  {
    Statistics.INSTANCE.startActivity(mActivity.get());
    MyTracker.onStartActivity(mActivity.get());
  }

  public void onStop()
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

  public void onPostResume()
  {
    if (mThemeName != null && mThemeName.equals(Config.getCurrentUiTheme()))
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
