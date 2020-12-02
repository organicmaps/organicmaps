package com.mapswithme.maps.settings;

import android.app.Activity;
import android.app.ProgressDialog;
import android.content.BroadcastReceiver;
import android.content.ContentResolver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Build;
import android.os.Environment;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AlertDialog;

import com.mapswithme.maps.BuildConfig;
import com.mapswithme.maps.Framework;
import com.mapswithme.maps.R;
import com.mapswithme.maps.downloader.MapManager;
import com.mapswithme.util.Config;
import com.mapswithme.maps.dialog.DialogUtils;
import com.mapswithme.util.concurrency.ThreadPool;
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
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.STORAGE);
  private static final String[] MOVABLE_EXTS = Framework.nativeGetMovableFilesExts();
  static final FilenameFilter MOVABLE_FILES_FILTER = new FilenameFilter()
  {
    @Override
    public boolean accept(File dir, String filename)
    {
      for (String ext : MOVABLE_EXTS)
        if (filename.endsWith(ext))
          return true;

      return false;
    }
  };

  interface MoveFilesListener
  {
    void moveFilesFinished(String newPath);

    void moveFilesFailed(int errorCode);
  }

  interface OnStorageListChangedListener
  {
    void onStorageListChanged(List<StorageItem> storageItems, int currentStorageIndex);
  }

  public static final int NO_ERROR = 0;
  public static final int UNKNOWN_LITE_PRO_ERROR = 1;
  public static final int IOEXCEPTION_ERROR = 2;
  public static final int NULL_ERROR = 4;
  public static final int NOT_A_DIR_ERROR = 5;
  public static final int UNKNOWN_KITKAT_ERROR = 6;

  static final String TAG = StoragePathManager.class.getName();

  private OnStorageListChangedListener mStoragesChangedListener;
  private MoveFilesListener mMoveFilesListener;

  private BroadcastReceiver mInternalReceiver;
  private Activity mActivity;
  private final List<StorageItem> mItems = new ArrayList<>();
  private int mCurrentStorageIndex = -1;

  /**
   * Observes status of connected media and retrieves list of available external storages.
   */
  void startExternalStorageWatching(Activity activity, final @Nullable OnStorageListChangedListener storagesChangedListener, @Nullable MoveFilesListener moveFilesListener)
  {
    mActivity = activity;
    mStoragesChangedListener = storagesChangedListener;
    mMoveFilesListener = moveFilesListener;
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

    mActivity.registerReceiver(mInternalReceiver, getMediaChangesIntentFilter());
    updateExternalStorages();
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

  void stopExternalStorageWatching()
  {
    if (mInternalReceiver != null)
    {
      mActivity.unregisterReceiver(mInternalReceiver);
      mInternalReceiver = null;
      mStoragesChangedListener = null;
    }
  }

  boolean hasMoreThanOneStorage()
  {
    return mItems.size() > 1;
  }

  List<StorageItem> getStorageItems()
  {
    return mItems;
  }

  int getCurrentStorageIndex()
  {
    return mCurrentStorageIndex;
  }

  private void updateExternalStorages()
  {
    updateExternalStorages(StorageUtils.getWritableDirRoot());
  }

  private void updateExternalStorages(String writableDir)
  {
    Set<String> pathsFromConfig = new HashSet<>();

    StorageUtils.parseStorages(pathsFromConfig);

    mItems.clear();

    final StorageItem currentStorage = buildStorageItem(writableDir);
    addStorageItem(currentStorage);
    addStorageItem(buildStorageItem(Environment.getExternalStorageDirectory().getAbsolutePath()));
    for (String path : pathsFromConfig)
      addStorageItem(buildStorageItem(path));

    mCurrentStorageIndex = mItems.indexOf(currentStorage);

    if (mCurrentStorageIndex == -1)
    {
      LOGGER.w(TAG, "Unrecognized current path : " + currentStorage);
      LOGGER.w(TAG, "Parsed paths : ");
      for (StorageItem item : mItems)
        LOGGER.w(TAG, item.toString());
    }
  }

  private void addStorageItem(StorageItem item)
  {
    if (item != null && !mItems.contains(item))
      mItems.add(item);
  }

  private static StorageItem buildStorageItem(String path)
  {
    try
    {
      final File f = new File(path + "/");
      if (f.exists() && f.isDirectory() && f.canWrite() && StorageUtils.isDirWritable(path))
      {
        final long freeSize = StorageUtils.getFreeBytesAtPath(path);
        if (freeSize > 0)
        {
          LOGGER.i(TAG, "Storage found : " + path + ", size : " + freeSize);
          return new StorageItem(path, freeSize);
        }
      }
    } catch (final IllegalArgumentException ex)
    {
      LOGGER.e(TAG, "Can't build storage for path : " + path, ex);
    }

    return null;
  }

  void changeStorage(int newIndex)
  {
    final StorageItem oldItem = (mCurrentStorageIndex != -1) ? mItems.get(mCurrentStorageIndex) : null;
    final StorageItem item = mItems.get(newIndex);
    final String path = item.getFullPath();

    final File f = new File(path);
    if (!f.exists() && !f.mkdirs())
    {
      LOGGER.e(TAG, "Can't create directory: " + path);
      return;
    }

    new AlertDialog.Builder(mActivity)
        .setCancelable(false)
        .setTitle(R.string.move_maps)
        .setPositiveButton(R.string.ok, new DialogInterface.OnClickListener()
        {
          @Override
          public void onClick(DialogInterface dlg, int which)
          {
            setStoragePath(mActivity, new MoveFilesListener()
            {
              @Override
              public void moveFilesFinished(String newPath)
              {
                updateExternalStorages();
                if (mMoveFilesListener != null)
                  mMoveFilesListener.moveFilesFinished(newPath);
              }

              @Override
              public void moveFilesFailed(int errorCode)
              {
                updateExternalStorages();
                if (mMoveFilesListener != null)
                  mMoveFilesListener.moveFilesFailed(errorCode);
              }
            }, item, oldItem, R.string.wait_several_minutes);
          }
        }).setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener()
        {
          @Override
          public void onClick(DialogInterface dlg, int which)
          {
            dlg.dismiss();
          }
        }).create().show();
  }

  /**
   * Checks whether current directory is actually writable on Kitkat devices. On earlier versions of android ( < 4.4 ) the root of external
   * storages was writable, but on Kitkat it isn't, so we should move our maps to other directory.
   * http://www.doubleencore.com/2014/03/android-external-storage/ check that link for explanations
   * <p/>
   * TODO : use SAF framework to allow custom sdcard folder selections on Lollipop+ devices.
   * https://developer.android.com/guide/topics/providers/document-provider.html#client
   * https://code.google.com/p/android/issues/detail?id=103249
   */
  public void checkKitkatMigration(@NonNull final Activity activity)
  {
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.KITKAT ||
        Config.isKitKatMigrationComplete())
      return;

    checkExternalStoragePathOnKitkat(activity, new MoveFilesListener()
    {
      @Override
      public void moveFilesFinished(String newPath)
      {
        Config.setKitKatMigrationComplete();
        DialogUtils.showAlertDialog(activity, R.string.kitkat_migrate_ok);
      }

      @Override
      public void moveFilesFailed(int errorCode)
      {
        DialogUtils.showAlertDialog(activity, R.string.kitkat_migrate_failed);
      }
    });
  }

  /**
   * Dumb way to determine whether the storage contains Maps.me data.
   * <p>The algorithm is quite simple:
   * <ul>
   *   <li>Find all writable storages;</li>
   *   <li>For each storage list sub-dirs under "MapsWithMe" dir;</li>
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

  public String findMapsMeStorage(String settingsPath)
  {
    updateExternalStorages(settingsPath);
    List<StorageItem> items = getStorageItems();

    for (StorageItem item : items)
    {
      if (containsMapData(item.mPath))
        return item.mPath;
    }

    return settingsPath;
  }

  private void checkExternalStoragePathOnKitkat(@NonNull Activity context, MoveFilesListener listener)
  {
    final String settingsDir = Framework.nativeGetSettingsDir();
    final String writableDir = Framework.nativeGetWritableDir();

    if (settingsDir.equals(writableDir) || StorageUtils.isDirWritable(writableDir))
      return;

    final long size = StorageUtils.getWritableDirSize();
    updateExternalStorages();
    for (StorageItem item : mItems)
    {
      if (item.mFreeSize > size)
      {
        setStoragePath(context, listener, item, new StorageItem(StorageUtils.getWritableDirRoot(), 0),
                       R.string.kitkat_optimization_in_progress);
        return;
      }
    }

    listener.moveFilesFailed(UNKNOWN_KITKAT_ERROR);
  }

  private void setStoragePath(@NonNull final Activity context,
                              @NonNull final MoveFilesListener listener,
                              @NonNull final StorageItem newStorage,
                              @Nullable final StorageItem oldStorage, final int messageId)
  {
    final ProgressDialog dialog = DialogUtils.createModalProgressDialog(context, messageId);
    dialog.show();

    ThreadPool.getStorage().execute(new Runnable()
    {
      @Override
      public void run()
      {
        final int result = changeStorage(newStorage, oldStorage);

        UiThread.run(new Runnable()
        {
          @Override
          public void run()
          {
            if (dialog.isShowing())
              dialog.dismiss();

            if (result == NO_ERROR)
              listener.moveFilesFinished(newStorage.mPath);
            else
              listener.moveFilesFailed(result);

            updateExternalStorages();
          }
        });
      }
    });
  }

  @SuppressWarnings("ResultOfMethodCallIgnored")
  private static int changeStorage(StorageItem newStorage, StorageItem oldStorage)
  {
    final String fullNewPath = newStorage.getFullPath();

    // According to changeStorage code above, oldStorage can be null.
    if (oldStorage == null)
    {
      LOGGER.w(TAG, "Old storage path is null. New path is: " + fullNewPath);
      return NULL_ERROR;
    }

    final File oldDir = new File(oldStorage.getFullPath());
    final File newDir = new File(fullNewPath);
    if (!newDir.exists())
      newDir.mkdir();

    if (BuildConfig.DEBUG)
    {
      if (!newDir.isDirectory())
        throw new IllegalStateException("Cannot move maps. New path is not a directory. New path : " + newDir);
      if (!oldDir.isDirectory())
        throw new IllegalStateException("Cannot move maps. Old path is not a directory. Old path : " + oldDir);
      if (!StorageUtils.isDirWritable(fullNewPath))
        throw new IllegalStateException("Cannot move maps. New path is not writable. New path : " + fullNewPath);
    }

    ArrayList<String> relPaths = new ArrayList<>();
    StorageUtils.listFilesRecursively(oldDir, "", MOVABLE_FILES_FILTER, relPaths);

    File[] oldFiles = new File[relPaths.size()];
    File[] newFiles = new File[relPaths.size()];
    for (int i = 0; i < relPaths.size(); ++i)
    {
      oldFiles[i] = new File(oldDir.getAbsolutePath() + File.separator + relPaths.get(i));
      newFiles[i] = new File(newDir.getAbsolutePath() + File.separator + relPaths.get(i));
    }

    try
    {
      for (int i = 0; i < oldFiles.length; ++i)
      {
        if (MapManager.nativeMoveFile(oldFiles[i].getAbsolutePath(), newFiles[i].getAbsolutePath()))
        {
          // No need to delete oldFiles[i] because it was moved to newFiles[i].
          oldFiles[i] = null;
        } else
        {
          File parent = newFiles[i].getParentFile();
          if (parent != null)
            parent.mkdirs();
          StorageUtils.copyFile(oldFiles[i], newFiles[i]);
        }
      }
    } catch (IOException e)
    {
      e.printStackTrace();
      // In the case of failure delete all new files.  Old files will
      // be lost if new files were just moved from old locations.
      StorageUtils.removeFilesInDirectory(newDir, newFiles);
      return IOEXCEPTION_ERROR;
    }

    UiThread.run(new Runnable()
    {
      @Override
      public void run()
      {
        Framework.nativeSetWritableDir(fullNewPath);
      }
    });

    // Delete old files because new files were successfully created.
    StorageUtils.removeFilesInDirectory(oldDir, oldFiles);
    return NO_ERROR;
  }
}
