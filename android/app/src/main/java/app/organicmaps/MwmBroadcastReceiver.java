package app.organicmaps;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

import androidx.annotation.NonNull;
import app.organicmaps.util.log.Logger;

public abstract class MwmBroadcastReceiver extends BroadcastReceiver
{
  private static final String TAG = MwmBroadcastReceiver.class.getSimpleName();

  @NonNull
  protected String getTag()
  {
    return getClass().getSimpleName();
  }

  protected abstract void onReceiveInitialized(@NonNull Context context, @NonNull Intent intent);

  @Override
  public final void onReceive(@NonNull Context context, @NonNull Intent intent)
  {
    MwmApplication app = MwmApplication.from(context);
    String msg = "onReceive: " + intent;
    Logger.i(TAG, msg);
    if (!app.arePlatformAndCoreInitialized())
    {
      Logger.w(TAG, "Application is not initialized, ignoring " + intent);
      return;
    }

    onReceiveInitialized(context, intent);
  }
}
