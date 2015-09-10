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
import android.support.annotation.Nullable;
import android.support.v7.app.AlertDialog;
import android.util.Log;

import com.mapswithme.maps.BuildConfig;
import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MapStorage;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.util.Constants;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.concurrency.ThreadPool;
import com.mapswithme.util.concurrency.UiThread;

import java.io.File;
import java.io.FileFilter;
import java.io.FilenameFilter;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Set;

public class StoragePathManager
{
  static final String[] MOVABLE_EXTS = Framework.nativeGetMovableFilesExts();
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

  public interface MoveFilesListener
  {
    void moveFilesFinished(String newPath);

    void moveFilesFailed(int errorCode);
  }

  public static final int NO_ERROR = 0;
  public static final int UNKNOWN_LITE_PRO_ERROR = 1;
  public static final int IOEXCEPTION_ERROR = 2;
  public static final int NULL_ERROR = 4;
  public static final int NOT_A_DIR_ERROR = 5;
  public static final int UNKNOWN_KITKAT_ERROR = 6;

  static final String TAG = StoragePathManager.class.getName();

  private static final String LITE_SDCARD_PREFIX = "Android/data/com.mapswithme.maps/files";
  private static final String SAMSUNG_LITE_SDCARD_PREFIX = "Android/data/com.mapswithme.maps.samsung/files";
  private static final String PRO_SDCARD_PREFIX = "Android/data/com.mapswithme.maps.pro/files";
  private static final String IS_KML_PLACED_IN_MAIN_STORAGE = "KmlBeenMoved";
  private static final String IS_KITKAT_MIGRATION_COMPLETED = "KitKatMigrationCompleted";

  private BroadcastReceiver mExternalReceiver;
  private BroadcastReceiver mInternalReceiver;
  private Activity mActivity;
  private ArrayList<StorageItem> mItems;
  private int mCurrentStorageIndex = -1;
  private MoveFilesListener mStorageListener;

  /**
   * Observes status of connected media and retrieves list of available external storages.
   */
  public void startExternalStorageWatching(Activity activity, @Nullable BroadcastReceiver receiver, @Nullable MoveFilesListener listener)
  {
    mActivity = activity;
    mStorageListener = listener;
    mExternalReceiver = receiver;
    mInternalReceiver = new BroadcastReceiver()
    {
      @Override
      public void onReceive(Context context, Intent intent)
      {
        if (mExternalReceiver != null)
          mExternalReceiver.onReceive(context, intent);

        updateExternalStorages();
      }
    };

    mActivity.registerReceiver(mInternalReceiver, getMediaChangesIntentFilter());
    updateExternalStorages();
  }

  public static IntentFilter getMediaChangesIntentFilter()
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
      mActivity.unregisterReceiver(mInternalReceiver);
      mInternalReceiver = null;
      mExternalReceiver = null;
    }
  }

  public boolean hasMoreThanOneStorage()
  {
    return mItems.size() > 1;
  }

  public ArrayList<StorageItem> getStorageItems()
  {
    return mItems;
  }

  public int getCurrentStorageIndex()
  {
    return mCurrentStorageIndex;
  }

  public void updateExternalStorages()
  {
    List<String> pathsFromConfig = new ArrayList<>();

    if (Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.KITKAT)
      StorageUtils.parseKitkatStorages(pathsFromConfig);
    else
      StorageUtils.parseStorages(pathsFromConfig);

    mItems = new ArrayList<>();

    final StorageItem currentStorage = buildStorageItem(StorageUtils.getWritableDirRoot());
    addStorageItem(currentStorage);
    addStorageItem(buildStorageItem(Environment.getExternalStorageDirectory().getAbsolutePath()));
    for (String path : pathsFromConfig)
      addStorageItem(buildStorageItem(path));

    mCurrentStorageIndex = mItems.indexOf(currentStorage);

    if (mCurrentStorageIndex == -1)
    {
      Log.w(TAG, "Unrecognized current path : " + currentStorage);
      Log.w(TAG, "Parsed paths : ");
      for (StorageItem item : mItems)
        Log.w(TAG, item.toString());
    }
  }

  private void addStorageItem(StorageItem item)
  {
    if (item != null && !mItems.contains(item))
      mItems.add(item);
  }

  private StorageItem buildStorageItem(String path)
  {
    try
    {
      final File f = new File(path + "/");
      if (f.exists() && f.isDirectory() && f.canWrite() && StorageUtils.isDirWritable(path))
      {
        final long freeSize = StorageUtils.getFreeBytesAtPath(path);
        if (freeSize > 0)
        {
          Log.i(TAG, "Storage found : " + path + ", size : " + freeSize);
          return new StorageItem(path, freeSize);
        }
      }
    } catch (final IllegalArgumentException ex)
    {
      Log.i(TAG, "Can't build storage for path : " + path);
    }

    return null;
  }

  @SuppressWarnings("ResultOfMethodCallIgnored")
  public boolean moveBookmarksToPrimaryStorage()
  {
    ArrayList<String> paths = new ArrayList<>();
    StorageUtils.parseStorages(paths);

    List<String> approvedPaths = new ArrayList<>();
    for (String path : paths)
    {
      String mwmPath = path + Constants.MWM_DIR_POSTFIX;
      File f = new File(mwmPath);
      if (f.exists() || f.canRead() || f.isDirectory())
        approvedPaths.add(mwmPath);
    }
    final String settingsDir = Framework.nativeGetSettingsDir();
    final String writableDir = Framework.nativeGetWritableDir();
    final String bookmarkDir = Framework.nativeGetBookmarkDir();
    final String bookmarkFileExt = Framework.nativeGetBookmarksExt();

    Set<File> bookmarks = new LinkedHashSet<>();
    if (!settingsDir.equals(writableDir))
      approvedPaths.add(writableDir);

    for (String path : approvedPaths)
    {
      if (!path.equals(settingsDir))
        accumulateFiles(path, bookmarkFileExt, bookmarks);
    }

    long bookmarksSize = 0;
    for (File f : bookmarks)
      bookmarksSize += f.length();

    if (StorageUtils.getFreeBytesAtPath(bookmarkDir) < bookmarksSize)
      return false;

    for (File oldBookmark : bookmarks)
    {
      String newBookmarkPath = BookmarkManager.generateUniqueBookmarkName(oldBookmark.getName().replace(bookmarkFileExt, ""));
      try
      {
        StorageUtils.copyFile(oldBookmark, new File(newBookmarkPath));
        oldBookmark.delete();
      } catch (IOException e)
      {
        e.printStackTrace();
        return false;
      }
    }

    Framework.nativeLoadBookmarks();

    return true;
  }

  private static void accumulateFiles(final String dirPath, final String filesExtension, Set<File> result)
  {
    File f = new File(dirPath);
    File[] bookmarks = f.listFiles(new FileFilter()
    {
      @Override
      public boolean accept(File pathname)
      {
        return pathname.getName().endsWith(filesExtension);
      }
    });

    result.addAll(Arrays.asList(bookmarks));
  }

  protected void changeStorage(int newIndex)
  {
    final StorageItem oldItem = (mCurrentStorageIndex != -1) ? mItems.get(mCurrentStorageIndex) : null;
    final StorageItem item = mItems.get(newIndex);
    final String path = item.getFullPath();

    final File f = new File(path);
    if (!f.exists() && !f.mkdirs())
    {
      Log.e(TAG, "Can't create directory: " + path);
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
                if (mStorageListener != null)
                  mStorageListener.moveFilesFinished(newPath);
              }

              @Override
              public void moveFilesFailed(int errorCode)
              {
                updateExternalStorages();
                if (mStorageListener != null)
                  mStorageListener.moveFilesFailed(errorCode);
              }
            }, item, oldItem, R.string.wait_several_minutes);

            dlg.dismiss();
          }
        })
        .setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener()
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
   */
  public void checkExternalStoragePathOnKitkat(Context context, MoveFilesListener listener)
  {
    if (Build.VERSION.SDK_INT < android.os.Build.VERSION_CODES.KITKAT)
      return;

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

  public void moveMapsLiteToPro(Context context, MoveFilesListener listener)
  {
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.KITKAT || !containsLiteMapsOnSdcard())
      return;

    final long size = StorageUtils.getWritableDirSize();
    final StorageItem currentStorage = new StorageItem(StorageUtils.getWritableDirRoot(), 0);

    // there is no need to copy maps from primary external storage(on internal device flash memory) -
    // maps are stored there in root folder and pro version can simply use them
    if (Environment.getExternalStorageDirectory().getAbsolutePath().equals(currentStorage.mPath))
      return;

    updateExternalStorages();
    for (StorageItem item : mItems)
    {
      if (item.mFreeSize > size && item.mPath.contains(PRO_SDCARD_PREFIX) && !item.mPath.equals(currentStorage.mPath))
      {
        setStoragePath(context, listener, item, currentStorage, R.string.move_lite_maps_to_pro);
        return;
      }
    }

    listener.moveFilesFailed(UNKNOWN_LITE_PRO_ERROR);
  }

  private boolean containsLiteMapsOnSdcard()
  {
    final String storagePath = StorageUtils.getWritableDirRoot();
    return storagePath.contains(LITE_SDCARD_PREFIX) || storagePath.contains(SAMSUNG_LITE_SDCARD_PREFIX);
  }

  /**
   * Checks bookmarks and data(mwms, routing, indexes etc) locations on external storages.
   * <p/>
   * Bookmarks should be placed in main MapsWithMe directory on primary storage (eg. SettingsDir, where settings.ini file is placed). If they were copied
   * to external storage (can happen on 2.6 and earlier mapswithme application versions) - we should move them back.
   * <p/>
   * Data should be placed in private app directory on Kitkat+ devices, hence root of sdcard isn't writable anymore there.
   */
  public void checkKitkatMigration(final Activity activity)
  {
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.KITKAT)
      return;

    migrateBookmarks(activity, new MoveFilesListener()
    {
      @Override
      public void moveFilesFinished(String newPath)
      {
        migrateMaps(activity);
      }

      @Override
      public void moveFilesFailed(int errorCode)
      {
        UiUtils.showAlertDialog(activity, R.string.bookmark_move_fail);
      }
    });
  }

  private void migrateBookmarks(final Activity activity, final MoveFilesListener listener)
  {
    if (MwmApplication.get().nativeGetBoolean(IS_KML_PLACED_IN_MAIN_STORAGE, false))
      listener.moveFilesFinished("");
    else
    {
      ThreadPool.getStorage().execute(new Runnable()
      {
        @Override
        public void run()
        {
          final boolean res = moveBookmarksToPrimaryStorage();

          UiThread.run(new Runnable()
          {
            @Override
            public void run()
            {
              if (res)
              {
                MwmApplication.get().nativeSetBoolean(IS_KML_PLACED_IN_MAIN_STORAGE, true);
                listener.moveFilesFinished("");
              }
              else
                listener.moveFilesFailed(NULL_ERROR);
            }
          });
        }
      });
    }
  }

  private void migrateMaps(final Activity activity)
  {
    if (MwmApplication.get().nativeGetBoolean(IS_KITKAT_MIGRATION_COMPLETED, false))
      return;

    checkExternalStoragePathOnKitkat(activity,
                                     new MoveFilesListener()
                                     {
                                       @Override
                                       public void moveFilesFinished(String newPath)
                                       {
                                         MwmApplication.get().nativeSetBoolean(IS_KITKAT_MIGRATION_COMPLETED, true);
                                         UiUtils.showAlertDialog(activity, R.string.kitkat_migrate_ok);
                                       }

                                       @Override
                                       public void moveFilesFailed(int errorCode)
                                       {
                                         UiUtils.showAlertDialog(activity, R.string.kitkat_migrate_failed);
                                       }
                                     }
    );
  }

  private void setStoragePath(final Context context, final MoveFilesListener listener, final StorageItem newStorage,
                              final StorageItem oldStorage, final int messageId)
  {
    final ProgressDialog dialog = new ProgressDialog(context);
    dialog.setMessage(context.getString(messageId));
    dialog.setProgressStyle(ProgressDialog.STYLE_SPINNER);
    dialog.setIndeterminate(true);
    dialog.setCancelable(false);
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
  private int changeStorage(StorageItem newStorage, StorageItem oldStorage)
  {
    final String fullNewPath = newStorage.getFullPath();

    // According to changeStorage code above, oldStorage can be null.
    if (oldStorage == null)
    {
      Log.w(TAG, "Old storage path is null. New path is: " + fullNewPath);
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
        if (!MapStorage.nativeMoveFile(oldFiles[i].getAbsolutePath(), newFiles[i].getAbsolutePath()))
        {
          File parent = newFiles[i].getParentFile();
          if (parent != null)
            parent.mkdirs();
          StorageUtils.copyFile(oldFiles[i], newFiles[i]);
        }
        else
        {
          // No need to delete oldFiles[i] because it was moved to newFiles[i].
          oldFiles[i] = null;
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
