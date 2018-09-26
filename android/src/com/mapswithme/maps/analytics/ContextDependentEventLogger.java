package com.mapswithme.maps.analytics;

import android.app.Application;
import android.support.annotation.NonNull;

abstract class ContextDependentEventLogger implements EventLogger
{
  @NonNull
  private final Application mApplication;

  ContextDependentEventLogger(@NonNull Application application)
  {
    mApplication = application;
  }

  @NonNull
  public Application getApplication()
  {
    return mApplication;
  }
}
