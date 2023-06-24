package app.organicmaps.base;

import android.content.Intent;
import android.text.TextUtils;
import android.util.Log;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.util.Config;
import app.organicmaps.util.CrashlyticsUtils;
import app.organicmaps.util.Utils;
import app.organicmaps.util.concurrency.UiThread;
import app.organicmaps.util.log.Logger;

public class BaseActivityDelegate
{
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
  }

  public void onPostCreate()
  {
    logLifecycleMethod("onPostCreate()");
  }

  public void onStart()
  {
    logLifecycleMethod("onStart()");
  }

  public void onStop()
  {
    logLifecycleMethod("onStop()");
  }

  public void onResume()
  {
    logLifecycleMethod("onResume()");
    Utils.showOnLockScreen(Config.isShowOnLockScreenEnabled(), mActivity.get());
  }

  public void onPause()
  {
    logLifecycleMethod("onPause()");
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
    Logger.i(TAG, msg);
  }
}
