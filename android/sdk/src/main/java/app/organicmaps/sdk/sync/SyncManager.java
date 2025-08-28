package app.organicmaps.sdk.sync;

import android.content.Context;
import androidx.annotation.Keep;
import androidx.annotation.MainThread;
import androidx.annotation.NonNull;
import androidx.lifecycle.DefaultLifecycleObserver;
import androidx.lifecycle.LifecycleOwner;
import androidx.lifecycle.ProcessLifecycleOwner;
import androidx.work.WorkManager;
import app.organicmaps.sdk.sync.engine.LockAlreadyHeldException;
import app.organicmaps.sdk.sync.engine.Syncer;
import app.organicmaps.sdk.sync.preferences.SyncCallback;
import app.organicmaps.sdk.sync.preferences.SyncPrefs;
import app.organicmaps.sdk.sync.preferences.SyncPrefsImpl;
import app.organicmaps.sdk.util.FileUtils;
import app.organicmaps.sdk.util.StorageUtils;
import app.organicmaps.sdk.util.concurrency.ThreadPool;
import app.organicmaps.sdk.util.concurrency.UiThread;
import app.organicmaps.sdk.util.log.Logger;
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
import java.util.stream.Collectors;

public enum SyncManager
{
  INSTANCE;
  public static final String TAG = SyncManager.class.getSimpleName();
  public static final String KML_EXTENSION = ".kml";

  private SyncPrefs mSyncPrefs;
  private SyncScheduler mSyncScheduler;
  private File mTempDir;
  private final Map<SyncAccount, Syncer> mSyncers = new ConcurrentHashMap<>();

  private final DefaultLifecycleObserver mLifecycleObserver = new DefaultLifecycleObserver() {
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

  public SyncPrefs getPrefs()
  {
    return mSyncPrefs;
  }

  public int countActiveAccounts()
  {
    return mSyncers.size();
  }

  public void initialize(Context context)
  {
    nativeInit();

    mSyncPrefs = new SyncPrefsImpl(context);
    mTempDir = new File(StorageUtils.getTempPath(context));
    for (SyncAccount account : mSyncPrefs.getEnabledAccounts())
      mSyncers.put(account, new Syncer(context, account));

    final Context appContext = context.getApplicationContext();
    mSyncPrefs.registerAccountToggledCallback(new SyncCallback.AccountToggled() {
      @Override
      public void onAccountEnabled(SyncAccount account)
      {
        mSyncers.put(account, new Syncer(appContext, account));
        if (countActiveAccounts() == 1)
          // addObserver automatically emits onStart event, so no need to fire it manually.
          ProcessLifecycleOwner.get().getLifecycle().addObserver(mLifecycleObserver);
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

  void syncAllAccounts()
  {
    Logger.i(TAG, "Started syncAllAccounts");
    for (Map.Entry<SyncAccount, Syncer> entry : mSyncers.entrySet())
    {
      try
      {
        entry.getValue().performSync();
        mSyncPrefs.setLastSynced(entry.getKey().getAccountId(), System.currentTimeMillis());
      }
      catch (LockAlreadyHeldException e)
      {
        Logger.e(TAG, "Lock held by another device", e);
        // TODO(savsch) implement retry policy (low priority as this clause should execute rarely)
      }
      catch (Exception e)
      {
        Logger.e(TAG, "Error syncing " + entry.getKey().getAuthState().getClass().getSimpleName(), e);
        mSyncPrefs.setErrorInfo(entry.getKey().getAccountId(),
                                e instanceof SyncOpException
                                    ? (SyncOpException) e
                                    : new SyncOpException.UnexpectedException(e.getLocalizedMessage()));
      }
    }
    mSyncPrefs.setLastRun(System.currentTimeMillis());
  }

  public File getBookmarksDir()
  {
    return new File(nativeGetBookmarksDir());
  }

  public File getTempDir()
  {
    return mTempDir;
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
  public void notifyOtherSyncers(int accountId, String filePath)
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
    if (FileUtils.deleteFile(filePath))
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
        //   the canonical path may
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
}
