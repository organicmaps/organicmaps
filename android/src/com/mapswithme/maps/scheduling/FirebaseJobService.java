package com.mapswithme.maps.scheduling;

import com.firebase.jobdispatcher.JobParameters;
import com.firebase.jobdispatcher.JobService;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

public class FirebaseJobService extends JobService
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
  private static final String TAG = NativeJobService.class.getSimpleName();

  @Override
  public boolean onStartJob(JobParameters job)
  {
    LOGGER.d(TAG, "onStartJob FirebaseJobService");
    JobServiceDelegate delegate = new JobServiceDelegate(getApplication());
    delegate.onStartJob();
    return true;
  }

  @Override
  public boolean onStopJob(JobParameters job)
  {
    return false;
  }
}
