package com.mapswithme.maps.scheduling;

import android.annotation.TargetApi;
import android.app.job.JobInfo;
import android.app.job.JobParameters;
import android.app.job.JobScheduler;
import android.app.job.JobService;
import android.content.ComponentName;
import android.os.Build;

import com.mapswithme.maps.background.NotificationService;

@TargetApi(Build.VERSION_CODES.LOLLIPOP)
public class NativeJobService extends JobService
{
  @Override
  public boolean onStartJob(JobParameters params)
  {
    JobDispatcherComposite jobDispatcher = JobDispatcherComposite.from(this);
    JobDispatcherComposite.NetworkStatus status = jobDispatcher.getNetworkStatus();
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
    /*FIXME*/
    JobInfo jobInfo = new JobInfo.Builder(jobId, component)
        .setMinimumLatency(4000)
        .setRequiredNetworkType(JobInfo.NETWORK_TYPE_ANY)
        .build();
    service.schedule(jobInfo);
  }

  @Override
  public boolean onStopJob(JobParameters params)
  {
    return false;
  }
}
