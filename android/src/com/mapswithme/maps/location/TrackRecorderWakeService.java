package com.mapswithme.maps.location;

import android.app.job.JobInfo;
import android.app.job.JobScheduler;
import android.content.Context;
import android.content.Intent;
import androidx.annotation.NonNull;
import androidx.core.app.JobIntentService;
import android.util.Log;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.scheduling.JobIdMap;
import com.mapswithme.util.CrashlyticsUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

import java.util.Arrays;
import java.util.List;
import java.util.Objects;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

public class TrackRecorderWakeService extends JobIntentService
{
  private static final String TAG = TrackRecorderWakeService.class.getSimpleName();
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.TRACK_RECORDER);
  private static final Object sLock = new Object();
  private static TrackRecorderWakeService sService;
  private final CountDownLatch mWaitMonitor = new CountDownLatch(1);

  @Override
  protected void onHandleWork(@NonNull Intent intent)
  {
    String msg = "onHandleIntent: " + intent + " app in background = "
                 + !MwmApplication.backgroundTracker(getApplicationContext()).isForeground();
    LOGGER.i(TAG, msg);
    CrashlyticsUtils.INSTANCE.log(Log.INFO, TAG, msg);

    synchronized (sLock)
    {
      sService = this;
    }
    TrackRecorder.INSTANCE.onServiceStarted();

    try
    {
      long timeout = TrackRecorder.INSTANCE.getAwaitTimeout();
      LOGGER.d(TAG, "Timeout: " + timeout);

      if (!mWaitMonitor.await(timeout, TimeUnit.MILLISECONDS))
      {
        LOGGER.d(TAG, "TIMEOUT awaiting coordinates");
        TrackRecorder.INSTANCE.incrementAwaitTimeout();
      }
    } catch (InterruptedException ignored) {}

    synchronized (sLock)
    {
      sService = null;
    }

    TrackRecorder.INSTANCE.onServiceStopped();
  }

  public static void start(@NonNull Context context)
  {
    Context app = context.getApplicationContext();

    Intent intent = new Intent(app, TrackRecorderWakeService.class);
    final int jobId = JobIdMap.getId(TrackRecorderWakeService.class);
    if (Utils.isLollipopOrLater())
    {
      JobScheduler scheduler = (JobScheduler) context.getSystemService(Context.JOB_SCHEDULER_SERVICE);
      Objects.requireNonNull(scheduler);
      List<JobInfo> pendingJobs = scheduler.getAllPendingJobs();
      String jobsRepresentation = Arrays.toString(pendingJobs.toArray());
      for (JobInfo each : pendingJobs)
      {
        if (TrackRecorderWakeService.class.getName().equals(each.getService().getClassName()))
        {
          scheduler.cancel(each.getId());
          String logMsg = "Canceled job: " + each + ". All jobs: " + jobsRepresentation;
          CrashlyticsUtils.INSTANCE.log(Log.INFO, TAG, logMsg);
        }
      }
    }
    JobIntentService.enqueueWork(app, TrackRecorderWakeService.class, jobId, intent);
  }

  public static void stop()
  {
    LOGGER.d(TAG, "SVC.stop()");

    synchronized (sLock)
    {
      if (sService != null)
        sService.mWaitMonitor.countDown();
      else
        LOGGER.d(TAG, "SVC.stop() SKIPPED because (sService == null)");
    }
  }
}
