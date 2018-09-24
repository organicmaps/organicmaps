package com.mapswithme.maps.content;

import android.app.Activity;
import android.app.Application;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.mapswithme.maps.BuildConfig;
import com.mapswithme.maps.PrivateVariables;
import com.my.tracker.MyTracker;
import com.my.tracker.MyTrackerParams;

import java.util.Map;

public class MyTrackerEventLogger extends ContextDependentEventLogger
{
  MyTrackerEventLogger(@NonNull Application application)
  {
    super(application);
    initTracker();
  }

  @Override
  public void sendTags(@NonNull String tag, @Nullable String[] params)
  {
    /* Do nothing */
  }

  @Override
  public void logEvent(@NonNull String event, @NonNull Map<String, String> params)
  {
    MyTracker.trackEvent(event, params);
  }

  @Override
  public void startActivity(@NonNull Activity context)
  {
    MyTracker.onStartActivity(context);
  }

  @Override
  public void stopActivity(@NonNull Activity context)
  {
    MyTracker.onStopActivity(context);
  }

  private void initTracker()
  {
    MyTracker.setDebugMode(BuildConfig.DEBUG);
    MyTracker.createTracker(PrivateVariables.myTrackerKey(), getApplication());
    final MyTrackerParams myParams = MyTracker.getTrackerParams();
    if (myParams != null)
      myParams.setDefaultVendorAppPackage();
    MyTracker.initTracker();
  }
}
