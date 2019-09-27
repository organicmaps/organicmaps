package com.mapswithme.maps.scheduling;

import androidx.annotation.NonNull;

import com.firebase.jobdispatcher.JobParameters;
import com.firebase.jobdispatcher.JobService;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

public class FirebaseJobService extends JobService
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
  private static final String TAG = FirebaseJobService.class.getSimpleName();

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
  public boolean onStartJob(JobParameters job)
  {
    LOGGER.d(TAG, "onStartJob FirebaseJobService");
    return mDelegate.onStartJob();
  }

  @Override
  public boolean onStopJob(JobParameters job)
  {
    return mDelegate.onStopJob();
  }
}
