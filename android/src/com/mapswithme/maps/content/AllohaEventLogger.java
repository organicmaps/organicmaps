package com.mapswithme.maps.content;

import android.app.Activity;
import android.app.Application;
import android.support.annotation.NonNull;

import com.mapswithme.maps.BuildConfig;
import com.mapswithme.maps.PrivateVariables;

import java.util.Map;

class AllohaEventLogger extends DefaultEventLogger
{
  AllohaEventLogger(@NonNull Application application)
  {
    super(application);
    // At the moment, need to always initialize engine for correct JNI http part reusing.
    // Statistics is still enabled/disabled separately and never sent anywhere if turned off.
    // TODO (AlexZ): Remove this initialization dependency from JNI part.
    org.alohalytics.Statistics.setDebugMode(BuildConfig.DEBUG);
    org.alohalytics.Statistics.setup(PrivateVariables.alohalyticsUrl(), application);
  }

  @Override
  public void logEvent(@NonNull String event, @NonNull Map<String, String> params)
  {
    super.logEvent(event, params);
    org.alohalytics.Statistics.logEvent(event, params);
  }

  @Override
  public void startActivity(@NonNull Activity context)
  {
    super.startActivity(context);
    org.alohalytics.Statistics.onStart(context);

  }

  @Override
  public void stopActivity(@NonNull Activity context)
  {
    super.stopActivity(context);
    org.alohalytics.Statistics.onStop(context);
  }
}
