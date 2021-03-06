package com.mapswithme.maps.scheduling;

import android.app.Application;
import androidx.annotation.NonNull;

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
