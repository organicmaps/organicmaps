package com.mapswithme.maps.base;

import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Build;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.util.Config;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.ViewServer;
import com.mapswithme.util.concurrency.UiThread;
import com.mapswithme.util.statistics.Statistics;
import com.my.tracker.MyTracker;

import static android.Manifest.permission.WRITE_EXTERNAL_STORAGE;

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
    if (!MwmApplication.get().isPlatformInitialized() || !Utils.isWriteExternalGranted(mActivity.get()))
      return;
    Statistics.INSTANCE.startActivity(mActivity.get());
    MyTracker.onStartActivity(mActivity.get());
  }

  public void onStop()
  {
    if (!MwmApplication.get().isPlatformInitialized() || !Utils.isWriteExternalGranted(mActivity.get()))
      return;
    Statistics.INSTANCE.stopActivity(mActivity.get());
    MyTracker.onStopActivity(mActivity.get());
  }

  public void onResume()
  {
    if (!MwmApplication.get().isPlatformInitialized() || !Utils.isWriteExternalGranted(mActivity.get()))
      return;
    org.alohalytics.Statistics.logEvent("$onResume", mActivity.getClass().getSimpleName() + ":" +
                                                     UiUtils.deviceOrientationAsString(mActivity.get()));
    ViewServer.get(mActivity.get()).setFocusedWindow(mActivity.get());
  }

  public void onPause()
  {
    if (!MwmApplication.get().isPlatformInitialized() || !Utils.isWriteExternalGranted(mActivity.get()))
      return;
    org.alohalytics.Statistics.logEvent("$onPause", mActivity.getClass().getSimpleName());
  }

  public void onPostResume()
  {
    if (mThemeName == null || mThemeName.equals(Config.getCurrentUiTheme()) || !Utils.isWriteExternalGranted(mActivity.get()))
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
