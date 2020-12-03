package com.mapswithme.maps.scheduling;

import android.annotation.TargetApi;
import android.app.job.JobInfo;
import android.app.job.JobScheduler;
import android.content.ComponentName;
import android.content.Context;
import android.os.Build;

import androidx.annotation.NonNull;
import com.firebase.jobdispatcher.Constraint;
import com.firebase.jobdispatcher.FirebaseJobDispatcher;
import com.firebase.jobdispatcher.GooglePlayDriver;
import com.firebase.jobdispatcher.Job;
import com.firebase.jobdispatcher.Lifetime;
import com.firebase.jobdispatcher.Trigger;
import com.google.android.gms.common.ConnectionResult;
import com.google.android.gms.common.GoogleApiAvailability;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.util.CrashlyticsUtils;
import com.mapswithme.util.Utils;

import java.util.Objects;
import java.util.concurrent.TimeUnit;

public class ConnectivityJobScheduler implements ConnectivityListener
{
  private static final int SCHEDULE_PERIOD_IN_HOURS = 1;

  @NonNull
  private final ConnectivityListener mMasterConnectivityListener;

  public ConnectivityJobScheduler(@NonNull MwmApplication context)
  {
    mMasterConnectivityListener = Utils.isLollipopOrLater()
                                  ? createNativeJobScheduler(context)
                                  : createCompatJobScheduler(context);
  }

  @NonNull
  private ConnectivityListener createCompatJobScheduler(@NonNull MwmApplication context)
  {
    int status = GoogleApiAvailability.getInstance().isGooglePlayServicesAvailable(context);
    boolean isAvailable = status == ConnectionResult.SUCCESS;
    return isAvailable ? new ConnectivityListenerCompat(context) : new ConnectivityListenerStub();
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

  @TargetApi(Build.VERSION_CODES.LOLLIPOP)
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

  private static class ConnectivityListenerCompat implements ConnectivityListener
  {
    @NonNull
    private final FirebaseJobDispatcher mJobDispatcher;

    ConnectivityListenerCompat(@NonNull MwmApplication context)
    {
      mJobDispatcher = new FirebaseJobDispatcher(new GooglePlayDriver(context));
    }

    @Override
    public void listen()
    {
      String tag = String.valueOf(JobIdMap.getId(FirebaseJobService.class));
      int executionWindowStart = (int) TimeUnit.HOURS.toSeconds(SCHEDULE_PERIOD_IN_HOURS);
      Job job = mJobDispatcher.newJobBuilder()
                              .setTag(tag)
                              .setService(FirebaseJobService.class)
                              .setConstraints(Constraint.ON_ANY_NETWORK)
                              .setLifetime(Lifetime.FOREVER)
                              .setTrigger(Trigger.executionWindow(executionWindowStart, ++executionWindowStart))
                              .build();
      mJobDispatcher.mustSchedule(job);
    }
  }

  private static class ConnectivityListenerStub implements ConnectivityListener
  {
    ConnectivityListenerStub()
    {
      IllegalStateException exception = new IllegalStateException("Play services doesn't exist on" +
                                                                  " the device");
      CrashlyticsUtils.INSTANCE.logException(exception);
    }

    @Override
    public void listen()
    {
      /* Do nothing */
    }
  }

}
