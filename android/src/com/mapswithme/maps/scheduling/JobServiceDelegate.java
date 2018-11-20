package com.mapswithme.maps.scheduling;

import android.app.Application;
import android.support.annotation.NonNull;

import com.mapswithme.maps.background.NotificationService;
import com.mapswithme.util.ConnectionState;

class JobServiceDelegate
{
  @NonNull
  private final Application mApp;

  JobServiceDelegate(@NonNull Application app)
  {
    mApp = app;
  }

  boolean onStartJob()
  {
    ConnectionState.Type type = ConnectionState.requestCurrentType();
    if (type == ConnectionState.Type.WIFI)
      NotificationService.startOnConnectivityChanged(mApp);

    retryJob();
    return true;
  }

  private void retryJob()
  {
    ConnectivityJobScheduler.from(mApp).listen();
  }

  boolean onStopJob()
  {
    return false;
  }
}
