package com.mapswithme.maps.content;

import android.app.Activity;
import android.app.Application;
import android.support.annotation.NonNull;
import android.util.Log;

import com.flurry.android.FlurryAgent;
import com.mapswithme.maps.BuildConfig;
import com.mapswithme.maps.PrivateVariables;

import java.util.Map;

public class FlurryEventLogger extends DefaultEventLogger
{
  public FlurryEventLogger(@NonNull Application application)
  {
    super(application);
    initFlurry();
  }

  private void initFlurry()
  {
    //noinspection ConstantConditions
    FlurryAgent.setVersionName(BuildConfig.VERSION_NAME);
    new FlurryAgent
        .Builder()
        .withLogEnabled(true)
        .withLogLevel(BuildConfig.DEBUG ? Log.DEBUG : Log.ERROR)
        .withCaptureUncaughtExceptions(false)
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
    FlurryAgent.onEndSession(context.getApplicationContext());
  }

  @Override
  public void stopActivity(@NonNull Activity context)
  {
    super.stopActivity(context);
    FlurryAgent.onStartSession(context.getApplicationContext());
  }
}
