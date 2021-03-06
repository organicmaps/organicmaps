package com.mapswithme.maps.scheduling;

import android.app.job.JobInfo;
import android.app.job.JobScheduler;
import android.content.ComponentName;
import android.content.Context;

import androidx.annotation.NonNull;
import com.mapswithme.maps.MwmApplication;

import java.util.Objects;
import java.util.concurrent.TimeUnit;

public class ConnectivityJobScheduler implements ConnectivityListener
{
  private static final int SCHEDULE_PERIOD_IN_HOURS = 1;

  @NonNull
  private final ConnectivityListener mMasterConnectivityListener;

  public ConnectivityJobScheduler(@NonNull MwmApplication context)
  {
    mMasterConnectivityListener = createNativeJobScheduler(context);
  }

  @NonNull
  private ConnectivityListener createNativeJobScheduler(@NonNull MwmApplication context)
  {
    return new NativeConnectivityListener(context);
  }

  @Override
  public void listen()
  {
    mMasterConnectivityListener.listen();
  }

  @NonNull
  public static ConnectivityJobScheduler from(@NonNull Context context)
  {
    MwmApplication application = (MwmApplication) context.getApplicationContext();
    return (ConnectivityJobScheduler) application.getConnectivityListener();
  }

  private static class NativeConnectivityListener implements ConnectivityListener
  {
    @NonNull
    private final JobScheduler mJobScheduler;
    @NonNull
    private final Context mContext;

    NativeConnectivityListener(@NonNull Context context)
    {
      mContext = context;
      JobScheduler jobScheduler = (JobScheduler) mContext.getSystemService(Context.JOB_SCHEDULER_SERVICE);
      Objects.requireNonNull(jobScheduler);
      mJobScheduler = jobScheduler;
    }

    @Override
    public void listen()
    {
      ComponentName component = new ComponentName(mContext, NativeJobService.class);
      int jobId = JobIdMap.getId(NativeJobService.class);
      JobInfo jobInfo = new JobInfo
          .Builder(jobId, component)
          .setRequiredNetworkType(JobInfo.NETWORK_TYPE_ANY)
          .setPersisted(true)
          .setMinimumLatency(TimeUnit.HOURS.toMillis(SCHEDULE_PERIOD_IN_HOURS))
          .build();
      mJobScheduler.schedule(jobInfo);
    }
  }
}
