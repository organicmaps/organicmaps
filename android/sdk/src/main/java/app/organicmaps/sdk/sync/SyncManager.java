package app.organicmaps.sdk.sync;

import android.content.Context;
import androidx.annotation.NonNull;
import androidx.annotation.WorkerThread;
import androidx.lifecycle.DefaultLifecycleObserver;
import androidx.lifecycle.LifecycleOwner;
import androidx.lifecycle.ProcessLifecycleOwner;
import androidx.work.WorkManager;
import app.organicmaps.sdk.bookmarks.data.BookmarkManager;
import app.organicmaps.sdk.sync.engine.LockAlreadyHeldException;
import app.organicmaps.sdk.sync.engine.Syncer;
import app.organicmaps.sdk.sync.preferences.SyncCallback;
import app.organicmaps.sdk.sync.preferences.SyncPrefs;
import app.organicmaps.sdk.sync.preferences.SyncPrefsImpl;
import app.organicmaps.sdk.util.StorageUtils;
import app.organicmaps.sdk.util.log.Logger;
import java.io.File;
import java.io.IOException;
import java.util.Arrays;
import java.util.HashSet;
import java.util.Map;
import java.util.Objects;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;
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
        entry.getValue().performSync(mTempDir);
        mSyncPrefs.setLastSynced(entry.getKey().getAccountId(), System.currentTimeMillis());
      }
      catch (SyncOpException e)
      {
        mSyncPrefs.setErrorInfo(entry.getKey().getAccountId(), e);
        Logger.e(TAG, "Error syncing " + entry.getKey().getAuthState().getClass().getSimpleName(), e);
      }
      catch (LockAlreadyHeldException e)
      {
        Logger.e(TAG, "Lock held by another device", e);
        // TODO(savsch) implement retry policy (low priority as this clause should execute rarely)
      }
    }
    mSyncPrefs.setLastRun(System.currentTimeMillis());
  }

  /**
   * @param filePath the native representation of the file path. Must be preserved as-is
   *                 (i.e. /data/user/0/... must stay /data/user/0/... and not be changed
   *                 to /data/data/... even if both point to the same physical file),
   *                 for methods like BookmarkManager::ReloadBookmark to work as expected
   *                 when provided with this path.
   */
  @WorkerThread
  public void onFileChanged(String filePath)
  {
    for (Syncer syncer : mSyncers.values())
      syncer.markFileChanged(filePath);
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

  public Set<String> getLocalFilePaths()
  {
    File bookmarksDir = BookmarkManager.getBookmarksDir();
    Set<String> localFilePaths = new HashSet<>();
    try
    {
      String[] categoryPaths = BookmarkManager.getLoadedCategoryPaths();

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
}
