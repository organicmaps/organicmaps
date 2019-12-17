package com.mapswithme.maps.analytics;

import android.app.Activity;
import android.app.Application;
import androidx.annotation.NonNull;
import android.util.Log;

import com.flurry.android.FlurryAgent;
import com.mapswithme.maps.BuildConfig;
import com.mapswithme.maps.PrivateVariables;

import java.util.Map;

class FlurryEventLogger extends DefaultEventLogger
{
  FlurryEventLogger(@NonNull Application application)
  {
    super(application);
  }

  @Override
  public void initialize()
  {
    //noinspection ConstantConditions
    FlurryAgent.setVersionName(BuildConfig.VERSION_NAME);
    FlurryAgent.setDataSaleOptOut(true);
    new FlurryAgent
        .Builder()
        .withLogEnabled(true)
        .withLogLevel(BuildConfig.DEBUG ? Log.DEBUG : Log.ERROR)
        .withCaptureUncaughtExceptions(false)
        .withDataSaleOptOut(true)
        .build(getApplication(), PrivateVariables.flurryKey());
  }

  @Override
  public void logEvent(@NonNull String event, @NonNull Map<String, String> params)
  {
    super.logEvent(event, params);
    FlurryAgent.logEvent(event, params);
  }

  @Override
  public void startActivity(@NonNull Activity context)
  {
    super.startActivity(context);
    FlurryAgent.onStartSession(context.getApplicationContext());
  }

  @Override
  public void stopActivity(@NonNull Activity context)
  {
    super.stopActivity(context);
    FlurryAgent.onEndSession(context.getApplicationContext());
  }
}
