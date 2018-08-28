package com.mapswithme.maps.scheduling;

import android.annotation.TargetApi;
import android.app.job.JobInfo;
import android.app.job.JobParameters;
import android.app.job.JobScheduler;
import android.app.job.JobService;
import android.content.ComponentName;
import android.os.Build;

import com.mapswithme.maps.background.NotificationService;

import java.util.Objects;

@TargetApi(Build.VERSION_CODES.LOLLIPOP)
public class NativeJobService extends JobService
{
  @Override
  public boolean onStartJob(JobParameters params)
  {
    ConnectivityJobScheduler jobDispatcher = ConnectivityJobScheduler.from(this);
    ConnectivityJobScheduler.NetworkStatus status = jobDispatcher.getNetworkStatus();
    if (status.isNetworkStateChanged())
      NotificationService.startOnConnectivityChanged(this);

    if(android.os.Build.VERSION.SDK_INT >= Build.VERSION_CODES.N)
      scheduleRefresh();

    return true;
  }

  private void scheduleRefresh()
  {
    JobScheduler service = (JobScheduler) getSystemService(JOB_SCHEDULER_SERVICE);
    ComponentName component = new ComponentName(getApplicationContext(), NativeJobService.class);
    int jobId = NativeJobService.class.hashCode();
    JobInfo jobInfo = new JobInfo.Builder(jobId, component)
        .setMinimumLatency(ConnectivityJobScheduler.PERIODIC_IN_MILLIS)
        .setRequiredNetworkType(JobInfo.NETWORK_TYPE_ANY)
        .build();
    Objects.requireNonNull(service).schedule(jobInfo);
  }

  @Override
  public boolean onStopJob(JobParameters params)
  {
    return false;
  }
}
