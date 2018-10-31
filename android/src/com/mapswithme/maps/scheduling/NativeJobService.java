package com.mapswithme.maps.scheduling;

import android.annotation.TargetApi;
import android.app.job.JobParameters;
import android.app.job.JobService;
import android.os.Build;
import android.support.annotation.NonNull;

import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

@TargetApi(Build.VERSION_CODES.LOLLIPOP)
public class NativeJobService extends JobService
{
  private static final String TAG = NativeJobService.class.getSimpleName();

  @SuppressWarnings("NullableProblems")
  @NonNull
  private JobServiceDelegate mDelegate;

  @Override
  public void onCreate()
  {
    super.onCreate();
    mDelegate = new JobServiceDelegate(getApplication());
  }

  @Override
  public boolean onStartJob(JobParameters params)
  {
    Logger logger = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
    logger.d(TAG, "onStartJob");
    return mDelegate.onStartJob();
  }

  @Override
  public boolean onStopJob(JobParameters params)
  {
    return mDelegate.onStopJob();
  }
}
