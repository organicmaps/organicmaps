package app.organicmaps.sync;

import android.content.Context;
import androidx.annotation.Keep;
import androidx.annotation.MainThread;
import androidx.annotation.NonNull;
import androidx.lifecycle.DefaultLifecycleObserver;
import androidx.lifecycle.LifecycleOwner;
import androidx.lifecycle.ProcessLifecycleOwner;
import androidx.work.ExistingWorkPolicy;
import androidx.work.OneTimeWorkRequest;
import androidx.work.WorkManager;
import androidx.work.Worker;
import androidx.work.WorkerParameters;
import app.organicmaps.MwmApplication;
import app.organicmaps.sdk.util.concurrency.ThreadPool;
import app.organicmaps.sdk.util.concurrency.UiThread;
import app.organicmaps.sdk.util.log.Logger;
import app.organicmaps.util.FileUtils;
import app.organicmaps.util.InsecureHttpsHelper;
import java.io.File;
import java.io.IOException;
import java.util.Arrays;
import java.util.HashSet;
import java.util.Map;
import java.util.Objects;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.FutureTask;
import java.util.concurrent.TimeUnit;
import java.util.stream.Collectors;
import okhttp3.OkHttpClient;

public enum SyncManager
{
  INSTANCE;
  public static final String TAG = SyncManager.class.getSimpleName();
  public static final String KML_EXTENSION = ".kml";

  private SyncPrefs mSyncPrefs;
  private OkHttpClient mInsecureOkHttpClient;
  private SyncScheduler mSyncScheduler;
  private final Map<SyncAccount, Syncer> mSyncers = new ConcurrentHashMap<>();

  private DefaultLifecycleObserver mLifecycleObserver = new DefaultLifecycleObserver() {
    @Override
    public void onStart(@NonNull LifecycleOwner owner)
    {
      mSyncScheduler.onForeground();
    }

    @Override
    public void onStop(@NonNull LifecycleOwner owner)
    {
      mSyncScheduler.onBackground();
    }
  };

  @MainThread
  private static native void nativeReloadBookmark(@NonNull String filePath);

  @MainThread
  public static native void nativeDeleteBmCategory(@NonNull String filePath);

  private static native void nativeAddSuffixToCategory(@NonNull String filePath);

  private static native String nativeGetBookmarksDir();

  @MainThread
  private static native String[] nativeGetLoadedCategoryPaths();

  public static String[] getLoadedCategoryPaths()
  {
    if (UiThread.isUiThread())
      return nativeGetLoadedCategoryPaths();
    else
    {
      FutureTask<String[]> task = new FutureTask<>(SyncManager::nativeGetLoadedCategoryPaths);
      UiThread.run(task);
      try
      {
        return task.get();
      }
      catch (ExecutionException | InterruptedException e)
      {
        Logger.e(TAG, "Error retrieving category paths", e);
        return new String[0];
      }
    }
  }

  public OkHttpClient getInsecureOkHttpClient()
  {
    if (mInsecureOkHttpClient == null)
      mInsecureOkHttpClient = InsecureHttpsHelper.createInsecureOkHttpClient();
    return mInsecureOkHttpClient;
  }

  public int countActiveAccounts()
  {
    return mSyncers.size();
  }

  public void initialize(Context context)
  {
    nativeInit();

    mSyncPrefs = SyncPrefs.getInstance(context);
    for (SyncAccount account : mSyncPrefs.getEnabledAccounts())
      mSyncers.put(account, new Syncer(context, account));

    final Context appContext = context.getApplicationContext();
    mSyncPrefs.registerAccountToggledCallback(new SyncPrefs.AccountToggledCallback() {
      @Override
      public void onAccountEnabled(SyncAccount account)
      {
        mSyncers.put(account, new Syncer(appContext, account));
        if (countActiveAccounts() == 1)
        {
          mSyncScheduler.onForeground();
          ProcessLifecycleOwner.get().getLifecycle().addObserver(mLifecycleObserver);
        }
      }

      @Override
      public void onAccountDisabled(SyncAccount account)
      {
        Syncer syncer = mSyncers.get(account);
        if (syncer != null)
        {
          syncer.onSyncDisabled();
          mSyncers.remove(account);
          if (countActiveAccounts() == 0)
            ProcessLifecycleOwner.get().getLifecycle().removeObserver(mLifecycleObserver);
        }
      }
    });

    mSyncScheduler = new SyncScheduler(mSyncPrefs, WorkManager.getInstance(context));

    if (countActiveAccounts() > 0)
      ProcessLifecycleOwner.get().getLifecycle().addObserver(mLifecycleObserver);
  }

  private void syncAllAccounts()
  {
    for (Map.Entry<SyncAccount, Syncer> entry : mSyncers.entrySet())
    {
      try
      {
        entry.getValue().performSync();
        mSyncPrefs.setLastSynced(entry.getKey().getAccountId(), System.currentTimeMillis());
      }
      catch (SyncOpException e)
      {
        Logger.e(TAG, "sus", e); // TODO more robust handling
      }
      catch (Syncer.LockAlreadyHeldException e)
      {
        Logger.e(TAG, "sus", e); // TODO more robust handling
      }
    }
  }

  public File getBookmarksDir()
  {
    return new File(nativeGetBookmarksDir());
  }

  /**
   * @param filePath the native representation of the file path. Must be preserved as-is
   *                 (i.e. /data/user/0/... must stay /data/user/0/... and not be changed
   *                 to /data/data/... even if both point to the same physical file),
   *                 for methods like BookmarkManager::ReloadBookmark to work as expected
   *                 when provided with this path.
   */
  // Called from JNI
  @Keep
  @SuppressWarnings("unused")
  @MainThread
  public void onFileChanged(String filePath)
  {
    ThreadPool.getWorker().execute(() -> {
      for (Syncer syncer : mSyncers.values())
        syncer.markFileChanged(filePath);
    });
  }

  /**
   * @param accountId The account id managed by the calling syncer.
   */
  public void notifyOtherSyncers(long accountId, String filePath)
  {
    for (Map.Entry<SyncAccount, Syncer> entry : mSyncers.entrySet())
    {
      if (entry.getKey().getAccountId() != accountId)
        entry.getValue().markFileChanged(filePath);
    }
  }

  /**
   * Deletes the bookmark category in a manner that's thread safe and
   * also doesn't make native layer trigger {@link #onFileChanged(String)}.
   * Blocking call.
   * @param filePath file path of the bookmark category to delete
   * @return Whether deletion succeeded or not.
   */
  public static boolean deleteCategorySilent(String filePath)
  {
    if (FileUtils.deleteFileSafe(filePath))
    {
      UiThread.run(() -> nativeDeleteBmCategory(filePath));
      return true;
    }
    else
      return false;
  }

  public static void reloadBookmark(String filePath)
  {
    // https://github.com/organicmaps/organicmaps/pull/10888
    UiThread.run(() -> nativeReloadBookmark(filePath));
  }

  /**
   * Adds a suffix to the category name (as well as the file name). The original file is deleted.
   * <p>
   * {@link #onFileChanged(String)} is not triggered for the original filename, it's triggered only for
   * the new filename.
   * Blocks until complete. Must not be called from the main thread.
   */
  public static void addSuffixToCategory(@NonNull String filePath)
  {
    nativeAddSuffixToCategory(filePath);
  }

  public Set<String> getLocalFilePaths()
  {
    File bookmarksDir = getBookmarksDir();
    Set<String> localFilePaths = new HashSet<>();
    try
    {
      String[] categoryPaths = getLoadedCategoryPaths();

      File[] supportedFiles = bookmarksDir.listFiles((file, name) -> name.endsWith(KML_EXTENSION));
      if (supportedFiles == null)
        throw new IOException("Unable to list files in the bookmarks directory " + bookmarksDir.getPath());
      Set<String> supportedPaths = Arrays.stream(supportedFiles)
                                       .map(file -> {
                                         try
                                         {
                                           return file.getCanonicalPath();
                                         }
                                         catch (IOException e)
                                         {
                                           Logger.e(TAG, "An error that should be impossible.", e);
                                           return null;
                                         }
                                       })
                                       .filter(Objects::nonNull)
                                       .collect(Collectors.toUnmodifiableSet());

      for (String path : categoryPaths)
      {
        // BookmarkManager only loads every kml file from/to the bookmarks directory, so this check should be redundant
        // at the time of writing
        if (supportedPaths.contains(new File(path).getCanonicalPath()))
          localFilePaths.add(
              path); // It's necessary to use the same representation (path) as the one used by cpp core. For instance,
                     // the canonical path may
                     //   be of the form "/data/data/..." when `path` is "/data/user/0/...". It is required so that
                     //   methods like BookmarkManager::ReloadBookmark work as expected when provided with the path.
        else
          Logger.w(TAG, "A loaded category (" + path
                            + ") has a path/extension that's unsupported for sync."); // This should be impossible
      }
    }
    catch (IOException | SecurityException | NullPointerException e)
    {
      Logger.e(TAG, "An error that should be impossible.", e);
    }
    return localFilePaths;
  }

  private native void nativeInit();

  // singleton
  private static class SyncScheduler
  {
    private static final long MINIMUM_BACKGROUND_INTERVAL = 24 * 3600 * 1000L;
    private static final String SYNC_WORKER_NAME = "BookmarksSync";

    private final SyncPrefs mSyncPrefs;
    private final WorkManager mWorkManager;
    private static volatile long syncIntervalMs;
    private static volatile boolean backgroundMode = true;
    private static volatile boolean appInForeground = false;
    private static volatile boolean workerRunning = false;

    private SyncScheduler(SyncPrefs syncPrefs, WorkManager workManager)
    {
      mSyncPrefs = syncPrefs;
      mWorkManager = workManager;
      syncIntervalMs = syncPrefs.getSyncIntervalMs();
      syncPrefs.registerFrequencyChangeCallback(this::onIntervalChanged);
    }

    private void onForeground()
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

    private void onBackground()
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
      long initialDelay = syncIntervalMs - mSyncPrefs.getTimeSinceLastRun();
      OneTimeWorkRequest.Builder requestBuilder = new OneTimeWorkRequest.Builder(SyncWorker.class);
      if (initialDelay > 0)
        requestBuilder.setInitialDelay(initialDelay, TimeUnit.MILLISECONDS).build();
      OneTimeWorkRequest request = requestBuilder.build();

      if (workerRunning)
        return;

      backgroundMode = false;
      mWorkManager.enqueueUniqueWork(SYNC_WORKER_NAME, existingWorkPolicy, request);
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
        workerRunning = true;
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

        OneTimeWorkRequest request =
            new OneTimeWorkRequest.Builder(SyncWorker.class).setInitialDelay(interval, TimeUnit.MILLISECONDS).build();
        try
        {
          WorkManager.getInstance(MwmApplication.sInstance)
              .enqueueUniqueWork(SYNC_WORKER_NAME, ExistingWorkPolicy.REPLACE, request)
              .getResult()
              .get();
        }
        catch (ExecutionException | InterruptedException e)
        {
          backgroundMode = true; // so that the work is re-enqueued once the app comes in foreground again.
          Logger.e(TAG, "Unable to make sure that the next sync task got enqueued", e);
        }
        workerRunning = false;
        return Result.success();
      }
    }
  }
}
