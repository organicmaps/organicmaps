package com.mapswithme.maps.settings;

import android.app.Application;
import android.content.BroadcastReceiver;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Environment;
import android.text.TextUtils;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.mapswithme.maps.Framework;
import com.mapswithme.maps.downloader.MapManager;
import com.mapswithme.util.Config;
import com.mapswithme.util.StorageUtils;
import com.mapswithme.util.concurrency.UiThread;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

import java.io.File;
import java.io.FileFilter;
import java.io.FilenameFilter;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

public class StoragePathManager
{
  static final String TAG = StoragePathManager.class.getName();
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.STORAGE);
  private static final String[] MOVABLE_EXTS = Framework.nativeGetMovableFilesExts();
  static final FilenameFilter MOVABLE_FILES_FILTER = (dir, filename) ->
  {
    for (String ext : MOVABLE_EXTS)
      if (filename.endsWith(ext))
        return true;

    return false;
  };

  interface OnStorageListChangedListener
  {
    void onStorageListChanged(List<StorageItem> storageItems, int currentStorageIndex);
  }

  private OnStorageListChangedListener mStoragesChangedListener;
  private BroadcastReceiver mInternalReceiver;
  private Context mContext;
  private final List<StorageItem> mItems = new ArrayList<>();
  private int mCurrentStorageIndex = -1;

  public StoragePathManager(@NonNull Context context)
  {
    mContext = context;
  }

  protected void finalize() throws Throwable
  {
    // Make sure watchers are detached.
    stopExternalStorageWatching();
    super.finalize();
  }

  /**
   * Observes status of connected media and retrieves list of available external storages.
   *
   * TODO: ATM its used to update settings UI only - watch and handle storage changes constantly at the app level
   */
  public void startExternalStorageWatching(final @Nullable OnStorageListChangedListener storagesChangedListener)
  {
    mStoragesChangedListener = storagesChangedListener;
    mInternalReceiver = new BroadcastReceiver()
    {
      @Override
      public void onReceive(Context context, Intent intent)
      {
        updateExternalStorages();

        if (mStoragesChangedListener != null)
          mStoragesChangedListener.onStorageListChanged(mItems, mCurrentStorageIndex);
      }
    };

    mContext.registerReceiver(mInternalReceiver, getMediaChangesIntentFilter());
  }

  private static IntentFilter getMediaChangesIntentFilter()
  {
    final IntentFilter filter = new IntentFilter();
    filter.addAction(Intent.ACTION_MEDIA_MOUNTED);
    filter.addAction(Intent.ACTION_MEDIA_REMOVED);
    filter.addAction(Intent.ACTION_MEDIA_EJECT);
    filter.addAction(Intent.ACTION_MEDIA_SHARED);
    filter.addAction(Intent.ACTION_MEDIA_UNMOUNTED);
    filter.addAction(Intent.ACTION_MEDIA_BAD_REMOVAL);
    filter.addAction(Intent.ACTION_MEDIA_UNMOUNTABLE);
    filter.addAction(Intent.ACTION_MEDIA_CHECKING);
    filter.addAction(Intent.ACTION_MEDIA_NOFS);
    filter.addDataScheme(ContentResolver.SCHEME_FILE);

    return filter;
  }

  public void stopExternalStorageWatching()
  {
    if (mInternalReceiver != null)
    {
      mContext.unregisterReceiver(mInternalReceiver);
      mInternalReceiver = null;
      mStoragesChangedListener = null;
    }
  }

  public List<StorageItem> getStorageItems()
  {
    return mItems;
  }

  public int getCurrentStorageIndex()
  {
    return mCurrentStorageIndex;
  }

  /**
   * Updates the list of available external storages.
   *
   * The scan order is following:
   *
   *  1. Current directory from Config.
   *  2. Application-specific directories on shared/external storage devices.
   *  3. Application-specific directory on the internal memory.
   *
   * Directories are checked for the free space and the write access.
   */
  public void updateExternalStorages() throws AssertionError
  {
    List<File> candidates = new ArrayList<>();

    // Current directory.
    // Config.getStoragePath() can be empty on the first run.
    String configDir = Config.getStoragePath();
    if (!TextUtils.isEmpty(configDir))
      candidates.add(new File(configDir));

    // External storages (SD cards and other).
    for (File dir : mContext.getExternalFilesDirs(null))
    {
      // There was an evidence that `dir` can be null on some Samsungs.
      // https://github.com/organicmaps/organicmaps/issues/632
      if (dir == null)
        continue;

      //
      // If the contents of emulated storage devices are backed by a private user data partition,
      // then there is little benefit to apps storing data here instead of the private directories
      // returned by Context#getFilesDir(), etc.
      //
      boolean isStorageEmulated;
      try
      {
        isStorageEmulated = Environment.isExternalStorageEmulated(dir);
      }
      catch (IllegalArgumentException e)
      {
        // isExternalStorageEmulated may throw IllegalArgumentException
        // https://github.com/organicmaps/organicmaps/issues/538
        isStorageEmulated = false;
      }
      if (!isStorageEmulated)
        candidates.add(dir);
    }

    // Internal storage must always exists, but Android is unpredictable.
    // https://github.com/organicmaps/organicmaps/issues/632
    File internalDir = mContext.getFilesDir();
    if (internalDir != null)
      candidates.add(internalDir);

    if (candidates.isEmpty())
      throw new AssertionError("Can't find available storage");

    //
    // Update internal state.
    //
    LOGGER.i(TAG, "Begin scanning storages");
    mItems.clear();
    mCurrentStorageIndex = -1;
    Set<String> unique = new HashSet<>();
    for (File dir : candidates)
    {
      try
      {
        String path = dir.getCanonicalPath();
        // Add the trailing separator because the native code assumes that all paths have it.
        if (!path.endsWith(File.separator))
          path = path + File.separator;

        if (!dir.exists() || !dir.isDirectory())
        {
          LOGGER.i(TAG, "Rejected " + path + ": not a directory");
          continue;
        }
        if (!dir.canWrite())
        {
          LOGGER.i(TAG, "Rejected " + path + ": not writable");
          continue;
        }

        if (!unique.add(path))
        {
          LOGGER.i(TAG, "Rejected " + path + ": a duplicate");
          continue;
        }

        final long freeSize = StorageUtils.getFreeBytesAtPath(path);
        StorageItem item = new StorageItem(path, freeSize);
        if (!TextUtils.isEmpty(configDir) && configDir.equals(path))
        {
          mCurrentStorageIndex = mItems.size();
        }
        LOGGER.i(TAG, "Accepted " + path + ": " + freeSize + " bytes available");
        mItems.add(item);
      }
      catch (IllegalArgumentException | IOException ex)
      {
        LOGGER.e(TAG, "Rejected " + dir.getPath() + ": error", ex);
        continue;
      }
    }
    LOGGER.i(TAG, "End scanning storages");

    if (!TextUtils.isEmpty(configDir) && mCurrentStorageIndex == -1)
    {
      LOGGER.w(TAG, configDir + ": can't find configured directory in the list above!");
      for (StorageItem item : mItems)
        LOGGER.w(TAG, item.toString());
    }
  }

  /**
   * Dumb way to determine whether the storage contains Organic Maps data.
   * <p>The algorithm is quite simple:
   * <ul>
   *   <li>Find all writable storages;</li>
   *   <li>If there is a directory with version-like name (e.g. "160602")…</li>
   *   <li>…and it is not empty…</li>
   *   <li>…we got it!</li>
   * </ul>
   */
  private static boolean containsMapData(String storagePath)
  {
    File path = new File(storagePath);
    File[] candidates = path.listFiles(new FileFilter()
    {
      @Override
      public boolean accept(File pathname)
      {
        if (!pathname.isDirectory())
          return false;

        try
        {
          String name = pathname.getName();
          if (name.length() != 6)
            return false;

          int version = Integer.valueOf(name);
          return (version > 120000 && version <= 999999);
        } catch (NumberFormatException ignored) {}

        return false;
      }
    });

    return (candidates != null && candidates.length > 0 &&
            candidates[0].list().length > 0);
  }

  /**
   * Finds a first available storage with existing maps files.
   * Returns the best available option if no maps files found.
   * See updateExternalStorages() for the scan order details.
   */
  public static String findMapsStorage(@NonNull Application application)
  {
    StoragePathManager instance = new StoragePathManager(application);
    instance.updateExternalStorages();

    List<StorageItem> items = instance.getStorageItems();

    for (StorageItem item : items)
    {
      if (containsMapData(item.mPath))
      {
        LOGGER.i(TAG, "Found maps files at " + item.mPath);
        return item.mPath;
      }
      else
      {
        LOGGER.i(TAG, "No maps files found at " + item.mPath);
      }
    }

    // Use the first item by default.
    final String defaultDir = items.get(0).mPath;
    LOGGER.i(TAG, "Using default directory: " + defaultDir);
    return defaultDir;
  }

  /**
   * Moves map files.
   */
  @SuppressWarnings("ResultOfMethodCallIgnored")
  public static boolean moveStorage(@NonNull final String newPath, @NonNull final String oldPath)
  {
    LOGGER.i(TAG, "Begin moving maps from " + oldPath + " to " + newPath);

    final File oldDir = new File(oldPath);
    final File newDir = new File(newPath);

    ArrayList<String> relPaths = new ArrayList<>();
    StorageUtils.listFilesRecursively(oldDir, "", MOVABLE_FILES_FILTER, relPaths);

    File[] oldFiles = new File[relPaths.size()];
    File[] newFiles = new File[relPaths.size()];
    for (int i = 0; i < relPaths.size(); ++i)
    {
      oldFiles[i] = new File(oldDir.getAbsolutePath() + File.separator + relPaths.get(i));
      newFiles[i] = new File(newDir.getAbsolutePath() + File.separator + relPaths.get(i));
    }

    for (int i = 0; i < oldFiles.length; ++i)
    {
      LOGGER.i(TAG, "Moving " + oldFiles[i].getPath() + " to " + newFiles[i].getPath());
      File parent = newFiles[i].getParentFile();
      if (parent != null)
        parent.mkdirs();
      if (!MapManager.nativeMoveFile(oldFiles[i].getPath(), newFiles[i].getPath()))
      {
        LOGGER.e(TAG, "Failed to move " + oldFiles[i].getPath() + " to " + newFiles[i].getPath());
        // In the case of failure delete all new files.  Old files will
        // be lost if new files were just moved from old locations.
        // TODO: Delete old files only after all of them were copied to the new location.
        StorageUtils.removeFilesInDirectory(newDir, newFiles);
        return false;
      }
    }
    LOGGER.i(TAG, "End moving maps");

    UiThread.run(() -> Framework.nativeSetWritableDir(newPath));

    return true;
  }
}
