package app.organicmaps;

import android.content.Intent;
import android.util.Log;

import androidx.annotation.NonNull;
import androidx.core.app.JobIntentService;
import app.organicmaps.util.CrashlyticsUtils;
import app.organicmaps.util.log.Logger;

public abstract class MwmJobIntentService extends JobIntentService
{
  private static final String TAG = MwmJobIntentService.class.getSimpleName();

  @NonNull
  protected String getTag()
  {
    return getClass().getSimpleName();
  }

  protected abstract void onHandleWorkInitialized(@NonNull Intent intent);

  @Override
  protected void onHandleWork(@NonNull Intent intent)
  {
    MwmApplication app = MwmApplication.from(this);
    String msg = "onHandleWork: " + intent;
    Logger.i(TAG, msg);
    CrashlyticsUtils.INSTANCE.log(Log.INFO, getTag(), msg);
    if (!app.arePlatformAndCoreInitialized())
    {
      Logger.w(TAG, "Application is not initialized, ignoring " + intent);
      return;
    }

    onHandleWorkInitialized(intent);
  }
}
