package com.mapswithme.maps;

import android.content.Intent;
import android.util.Log;

import androidx.annotation.NonNull;
import androidx.core.app.JobIntentService;

import com.mapswithme.util.CrashlyticsUtils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

import java.io.IOException;

public abstract class MwmJobIntentService extends JobIntentService
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);

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
    LOGGER.i(getTag(), msg);
    CrashlyticsUtils.INSTANCE.log(Log.INFO, getTag(), msg);
    try
    {
      app.ensureCoreInitialized();
    } catch (IOException e)
    {
      LOGGER.e(getTag(), "Failed to initialize application, ignoring " + intent);
      return;
    }

    onHandleWorkInitialized(intent);
  }
}
