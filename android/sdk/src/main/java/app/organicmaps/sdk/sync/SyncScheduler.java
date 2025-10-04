package app.organicmaps.sdk.sync;

import android.content.Context;
import androidx.annotation.NonNull;
import androidx.work.Constraints;
import androidx.work.ExistingWorkPolicy;
import androidx.work.NetworkType;
import androidx.work.OneTimeWorkRequest;
import androidx.work.WorkManager;
import androidx.work.Worker;
import androidx.work.WorkerParameters;
import app.organicmaps.sdk.sync.preferences.SyncPrefs;
import app.organicmaps.sdk.util.concurrency.ThreadPool;
import app.organicmaps.sdk.util.log.Logger;
import java.util.Objects;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeUnit;

class SyncScheduler
{
  private static final String TAG = SyncScheduler.class.getSimpleName();
  private static final long MINIMUM_BACKGROUND_INTERVAL = 24 * 3600 * 1000L;
  private static final String SYNC_WORKER_NAME = "BookmarksSync";
  private static final Constraints constraints = new Constraints.Builder()
                                                     .setRequiredNetworkType(NetworkType.CONNECTED)
                                                     .build(); // TODO make it so as to respect the mobile data setting

  private final WorkManager mWorkManager;
  private static volatile long syncIntervalMs;
  private static volatile boolean backgroundMode = true;
  private static volatile boolean appInForeground = false;
  private static volatile boolean workerRunning = false;

  SyncScheduler(SyncPrefs syncPrefs, WorkManager workManager)
  {
    mWorkManager = workManager;
    syncIntervalMs = syncPrefs.getSyncIntervalMs();
    syncPrefs.registerFrequencyChangeCallback(this::onIntervalChanged);
  }

  void onForeground()
  {
    appInForeground = true;
    if (backgroundMode)
    {
      ExistingWorkPolicy existingWorkPolicy =
          syncIntervalMs >= MINIMUM_BACKGROUND_INTERVAL
              ? ExistingWorkPolicy.KEEP // Foreground and Background modes are the same if the condition is met
              : ExistingWorkPolicy.REPLACE;
      scheduleForegroundSync(existingWorkPolicy);
    }
  }

  void onBackground()
  {
    appInForeground = false;
  }

  private void onIntervalChanged(long newInterval)
  {
    syncIntervalMs = newInterval;
    scheduleForegroundSync(ExistingWorkPolicy.REPLACE);
  }

  private void scheduleForegroundSync(ExistingWorkPolicy existingWorkPolicy)
  {
    long initialDelay = syncIntervalMs - SyncManager.INSTANCE.getPrefs().getTimeSinceLastRun();
    OneTimeWorkRequest.Builder requestBuilder =
        new OneTimeWorkRequest.Builder(SyncWorker.class).setConstraints(constraints);
    if (initialDelay > 0)
      requestBuilder.setInitialDelay(initialDelay, TimeUnit.MILLISECONDS).build();
    OneTimeWorkRequest request = requestBuilder.build();

    if (workerRunning)
      return;

    backgroundMode = false;
    var future = mWorkManager.enqueueUniqueWork(SYNC_WORKER_NAME, existingWorkPolicy, request).getResult();
    future.addListener(() -> {
      try
      {
        Objects.requireNonNull(future.get()); // guaranteed to be Operation.State.SUCCESS if it doesn't throw
      }
      catch (Exception e)
      {
        backgroundMode = true;
        Logger.e(TAG, "Unable to schedule sync work request", e);
      }
    }, ThreadPool.getWorker());
  }

  public static class SyncWorker extends Worker
  {
    public SyncWorker(@NonNull Context context, @NonNull WorkerParameters workerParams)
    {
      super(context, workerParams);
    }

    @NonNull
    @Override
    public Result doWork()
    {
      if (workerRunning)
        return Result.success();

      workerRunning = true;
      try
      {
        SyncManager.INSTANCE.syncAllAccounts();

        if (SyncManager.INSTANCE.countActiveAccounts() < 1)
        {
          backgroundMode = true;
          return Result.success();
        }

        final long interval;
        if (appInForeground || syncIntervalMs >= MINIMUM_BACKGROUND_INTERVAL)
        {
          backgroundMode = false;
          interval = syncIntervalMs;
        }
        else
        {
          backgroundMode = true;
          interval = MINIMUM_BACKGROUND_INTERVAL;
        }

        OneTimeWorkRequest request = new OneTimeWorkRequest.Builder(SyncWorker.class)
                                         .setConstraints(constraints)
                                         .setInitialDelay(interval, TimeUnit.MILLISECONDS)
                                         .build();
        try
        {
          WorkManager.getInstance()
              .enqueueUniqueWork(SYNC_WORKER_NAME, ExistingWorkPolicy.APPEND_OR_REPLACE, request)
              .getResult()
              .get();
          // A log line saying "Work [ id=..., tags={ ...$SyncWorker } ] was cancelled" is intended here,
          // because of ExistingWorkPolicy.REPLACE.
        }
        catch (ExecutionException | InterruptedException e)
        {
          backgroundMode = true; // so that the work is re-enqueued once the app comes in foreground again.
          Logger.e(TAG, "Unable to make sure that the next sync task got enqueued", e);
        }
        return Result.success();
      }
      finally
      {
        workerRunning = false;
      }
    }
  }
}
