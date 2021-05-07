package com.mapswithme.maps;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

import androidx.annotation.NonNull;

import com.mapswithme.util.CrashlyticsUtils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

public abstract class MwmBroadcastReceiver extends BroadcastReceiver
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);

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
    LOGGER.i(getTag(), msg);
    CrashlyticsUtils.INSTANCE.log(Log.INFO, getTag(), msg);
    if (!app.arePlatformAndCoreInitialized())
    {
      LOGGER.w(getTag(), "Application is not initialized, ignoring " + intent);
      return;
    }

    onReceiveInitialized(context, intent);
  }
}
