package com.mapswithme.maps.content;

import android.app.Application;
import android.support.annotation.NonNull;

import com.mapswithme.maps.MwmApplication;

public class EventLoggerProvider
{
  @NonNull
  public static EventLogger obtainLogger(@NonNull Application application)
  {
    MwmApplication app = (MwmApplication) application;
    return app.getMediator().getEventLogger();
  }
}
