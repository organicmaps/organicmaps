package com.mapswithme.maps.settings;

import android.app.Activity;
import android.app.ProgressDialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.AsyncTask;
import android.os.Build;
import android.os.Environment;
import android.os.StatFs;
import android.support.v7.app.AlertDialog;
import android.util.Log;

import com.mapswithme.maps.BuildConfig;
import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MWMApplication;
import com.mapswithme.maps.MapStorage;
import com.mapswithme.maps.R;
import com.mapswithme.util.Constants;
import com.mapswithme.util.Utils;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileFilter;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.nio.channels.FileChannel;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

public class StoragePathManager
{
  public interface SetStoragePathListener
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

  private static String TAG = StoragePathManager.class.getName();

  private static final String LITE_SDCARD_PREFIX = "Android/data/com.mapswithme.maps/files";
  private static final String SAMSUNG_LITE_SDCARD_PREFIX = "Android/data/com.mapswithme.maps.samsung/files";
  private static final String PRO_SDCARD_PREFIX = "Android/data/com.mapswithme.maps.pro/files";
  private static final int VOLD_MODE = 1;
  private static final int MOUNTS_MODE = 2;

  private BroadcastReceiver mExternalReceiver;
  private BroadcastReceiver mInternalReceiver;
  private Activity mActivity;
  private ArrayList<StoragePathAdapter.StorageItem> mItems;
  private int mCurrentStorageIndex = -1;
  private SetStoragePathListener mStorageListener;

  /**
   * Observes status of connected media and retrieves list of available external storages.
   *
   * @param activity context
   * @param receiver receiver to get broadcasts of media events. can be null
   * @param listener listener to get notifications about maps transfers. can be null
   */
  public void startExternalStorageWatching(Activity activity, BroadcastReceiver receiver, SetStoragePathListener listener)
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
    filter.addDataScheme(Constants.Url.DATA_SCHEME_FILE);

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

  public ArrayList<StoragePathAdapter.StorageItem> getStorageItems()
  {
    return mItems;
  }

  public int getCurrentStorageIndex()
  {
    return mCurrentStorageIndex;
  }

  public void updateExternalStorages()
  {
    ArrayList<String> paths = new ArrayList<>();

    if (Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.KITKAT)
      parseKitkatStorages(paths);
    else
      parseStorages(paths);

    Map<Long, String> pathsSizesMap = new HashMap<>();

    // Add current path first!
    final String currentStorageDir = getWritableDirRoot();
    addStoragePathWithSize(currentStorageDir, pathsSizesMap);
    for (String path : paths)
      addStoragePathWithSize(path, pathsSizesMap);
    addStoragePathWithSize(Environment.getExternalStorageDirectory().getAbsolutePath(), pathsSizesMap);

    mItems = new ArrayList<>();
    mCurrentStorageIndex = -1;
    for (Map.Entry<Long, String> entry : pathsSizesMap.entrySet())
    {
      StoragePathAdapter.StorageItem item = new StoragePathAdapter.StorageItem(entry.getValue(), entry.getKey());
      mItems.add(item);
      if (item.mPath.equals(currentStorageDir))
        mCurrentStorageIndex = mItems.size() - 1;
    }

    if (mCurrentStorageIndex == -1)
    {
      Log.w(TAG, "Unrecognized current path: " + currentStorageDir);
      Log.w(TAG, "Parsed paths: " + Utils.mapPrettyPrint(pathsSizesMap));
    }
  }

  private static void parseKitkatStorages(List<String> paths)
  {
    File primaryStorage = MWMApplication.get().getExternalFilesDir(null);
    File[] storages = MWMApplication.get().getExternalFilesDirs(null);
    if (storages != null)
    {
      for (File f : storages)
      {
        // add only secondary dirs
        if (f != null && !f.equals(primaryStorage))
        {
          Log.i(TAG, "Additional storage path: " + f.getPath());
          paths.add(f.getPath());
        }
      }
    }

    ArrayList<String> testStorages = new ArrayList<>();
    parseStorages(testStorages);
    final String suffix = String.format(Constants.STORAGE_PATH, BuildConfig.APPLICATION_ID, Constants.FILES_DIR);
    for (String testStorage : testStorages)
    {
      Log.i(TAG, "Test storage from config files : " + testStorage);
      if (isDirWritable(testStorage))
      {
        Log.i(TAG, "Found writable storage : " + testStorage);
        paths.add(testStorage);
      }
      else
      {
        testStorage += suffix;
        File file = new File(testStorage);
        if (!file.exists()) // create directory for our package if it isn't created by any reason
        {
          Log.i(TAG, "Try to create MWM path");
          file.mkdirs();
          file = new File(testStorage);
          if (file.exists())
            Log.i(TAG, "Created!");
        }
        if (isDirWritable(testStorage))
        {
          Log.i(TAG, "Found writable storage : " + testStorage);
          paths.add(testStorage);
        }
      }
    }
  }

  private static void parseStorages(ArrayList<String> paths)
  {
    parseMountFile("/etc/vold.conf", VOLD_MODE, paths);
    parseMountFile("/etc/vold.fstab", VOLD_MODE, paths);
    parseMountFile("/system/etc/vold.fstab", VOLD_MODE, paths);
    parseMountFile("/proc/mounts", MOUNTS_MODE, paths);
  }

  /// @name Assume that MapsWithMe folder doesn't have inner folders and symbolic links.
  public static long getMwmDirSize()
  {
    final File dir = new File(Framework.nativeGetWritableDir());
    assert (dir.exists());
    assert (dir.isDirectory());

    final File[] files = dir.listFiles();
    if (files == null)
      return 0;

    long size = 0;
    for (final File f : files)
    {
      assert (f.isFile());
      size += f.length();
    }

    return size;
  }

  public boolean moveBookmarks()
  {
    ArrayList<String> paths = new ArrayList<>();
    if (Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.KITKAT)
      parseStorages(paths);

    ArrayList<String> approvedPaths = new ArrayList<>();
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

    LinkedHashSet<File> bookmarks = new LinkedHashSet<File>();
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

    if (getFreeBytesAtPath(bookmarkDir) < bookmarksSize)
      return false;

    for (File f : bookmarks)
    {
      String name = f.getName();
      name = name.replace(bookmarkFileExt, "");
      name = nativeGenerateUniqueBookmarkName(name);
      try
      {
        copyFile(f, new File(name));
        f.delete();
      } catch (IOException e)
      {
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

  protected void onStorageItemClick(int index)
  {
    final StoragePathAdapter.StorageItem oldItem = (mCurrentStorageIndex != -1) ? mItems.get(mCurrentStorageIndex) : null;
    final StoragePathAdapter.StorageItem item = mItems.get(index);
    final String path = getItemFullPath(item);

    final File f = new File(path);
    if (!f.exists() && !f.mkdirs())
    {
      Log.e(TAG, "Can't create directory: " + path);
      return;
    }

    new AlertDialog.Builder(mActivity).setCancelable(false).setTitle(R.string.move_maps)
        .setPositiveButton(R.string.ok, new DialogInterface.OnClickListener()
        {
          @Override
          public void onClick(DialogInterface dlg, int which)
          {
            setStoragePath(mActivity, new StoragePathManager.SetStoragePathListener()
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
   *
   * @param context
   * @param listener
   */
  public void checkWritableDir(Context context, SetStoragePathListener listener)
  {
    if (Build.VERSION.SDK_INT < android.os.Build.VERSION_CODES.KITKAT)
      return;

    final String settingsDir = Framework.nativeGetSettingsDir();
    final String writableDir = Framework.nativeGetWritableDir();

    if (settingsDir.equals(writableDir))
      return;

    if (isDirWritable(writableDir))
      return;

    final long size = getMwmDirSize();
    updateExternalStorages();
    for (StoragePathAdapter.StorageItem item : mItems)
    {
      if (item.mSize > size)
      {
        setStoragePath(context, listener, item, new StoragePathAdapter.StorageItem(getWritableDirRoot(), 0),
            R.string.kitkat_optimization_in_progress);
        return;
      }
    }

    listener.moveFilesFailed(UNKNOWN_KITKAT_ERROR);
  }

  public void moveMapsLiteToPro(Context context, SetStoragePathListener listener)
  {
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.KITKAT)
      return;

    final long size = getMwmDirSize();
    StoragePathAdapter.StorageItem currentStorage = new StoragePathAdapter.StorageItem(getWritableDirRoot(), 0);

    // there is no need to copy maps from primary external storage(on internal device flash memory) -
    // maps are stored there in root folder and pro version can simply use them
    if (Environment.getExternalStorageDirectory().getAbsolutePath().equals(currentStorage.mPath))
      return;

    updateExternalStorages();
    for (StoragePathAdapter.StorageItem item : mItems)
    {
      if (item.mSize > size && item.mPath.contains(PRO_SDCARD_PREFIX)
          && !item.mPath.equals(currentStorage.mPath))
      {
        setStoragePath(context, listener, item, currentStorage,
            R.string.move_lite_maps_to_pro);
        return;
      }
    }

    listener.moveFilesFailed(UNKNOWN_LITE_PRO_ERROR);
  }

  public boolean containsLiteMapsOnSdcard()
  {
    final String storagePath = getWritableDirRoot();
    return storagePath.contains(LITE_SDCARD_PREFIX) || storagePath.contains(SAMSUNG_LITE_SDCARD_PREFIX);
  }

  private void setStoragePath(Context context, SetStoragePathListener listener, StoragePathAdapter.StorageItem newStorage,
                              StoragePathAdapter.StorageItem oldStorage, int messageId)
  {
    MoveFilesTask task = new MoveFilesTask(context, listener, newStorage, oldStorage, messageId);
    task.execute("");
  }

  private static int doMoveMaps(StoragePathAdapter.StorageItem newStorage, StoragePathAdapter.StorageItem oldStorage)
  {
    final String fullNewPath = getItemFullPath(newStorage);

    // According to onStorageItemClick code above, oldStorage can be null.
    if (oldStorage == null)
    {
      Log.w(TAG, "Old storage path is null. New path is: " + fullNewPath);
      return NULL_ERROR;
    }

    final String fullOldPath = getItemFullPath(oldStorage);
    final File oldDir = new File(fullOldPath);
    final File newDir = new File(fullNewPath);
    if (!newDir.exists())
      newDir.mkdir();

    assert (isDirWritable(fullNewPath));
    assert (newDir.isDirectory());
    assert (oldDir.isDirectory());

    final String[] extensions = Framework.nativeGetMovableFilesExt();

    File[] internalFiles = oldDir.listFiles(new FileFilter()
    {

      @Override
      public boolean accept(File pathname)
      {
        for (String postfix : extensions)
        {
          if (pathname.getName().endsWith(postfix))
            return true;
        }
        Log.w(TAG, "Move maps. Old files dir is empty: " + oldDir);
        return false;
      }
    });

    // Strange thing: null only if oldDir is not a directory, but it fires according to the developer console.
    if (internalFiles == null)
    {
      Log.w(TAG, "Source path is not a directory: " + fullOldPath);
      return NOT_A_DIR_ERROR;
    }

    try
    {
      for (File moveFile : internalFiles)
      {
        if (!MapStorage.nativeMoveFile(moveFile.getAbsolutePath(), fullNewPath + moveFile.getName()))
          copyFile(moveFile, new File(fullNewPath + moveFile.getName()));
      }
    } catch (IOException e)
    {
      e.printStackTrace();
      for (File moveFile : internalFiles)
        new File(fullNewPath + moveFile.getName()).delete();
      return IOEXCEPTION_ERROR;
    }

    Framework.nativeSetWritableDir(fullNewPath);

    for (File moveFile : internalFiles)
      moveFile.delete();

    return NO_ERROR;
  }

  private static void copyFile(File source, File dest) throws IOException
  {
    FileChannel inputChannel = null;
    FileChannel outputChannel = null;
    try
    {
      inputChannel = new FileInputStream(source).getChannel();
      outputChannel = new FileOutputStream(dest).getChannel();
      outputChannel.transferFrom(inputChannel, 0, inputChannel.size());
    } finally
    {
      inputChannel.close();
      outputChannel.close();
    }
  }

  private class MoveFilesTask extends AsyncTask<String, Void, Integer>
  {
    private final ProgressDialog mDlg;
    private final StoragePathAdapter.StorageItem mNewStorage;
    private final StoragePathAdapter.StorageItem mOldStorage;
    private final SetStoragePathListener mListener;

    public MoveFilesTask(Context context, SetStoragePathListener listener, StoragePathAdapter.StorageItem newStorage,
                         StoragePathAdapter.StorageItem oldStorage, int messageID)
    {
      mNewStorage = newStorage;
      mOldStorage = oldStorage;
      mListener = listener;

      mDlg = new ProgressDialog(context);
      mDlg.setMessage(context.getString(messageID));
      mDlg.setProgressStyle(ProgressDialog.STYLE_SPINNER);
      mDlg.setIndeterminate(true);
      mDlg.setCancelable(false);
    }

    @Override
    protected void onPreExecute()
    {
      mDlg.show();
    }

    @Override
    protected Integer doInBackground(String... params)
    {
      return doMoveMaps(mNewStorage, mOldStorage);
    }

    @Override
    protected void onPostExecute(Integer result)
    {
      // Using dummy try-catch because of the following:
      // http://stackoverflow.com/questions/2745061/java-lang-illegalargumentexception-view-not-attached-to-window-manager
      try
      {
        mDlg.dismiss();
      } catch (final Exception e)
      {
        e.printStackTrace();
      }

      if (result == NO_ERROR)
        mListener.moveFilesFinished(mNewStorage.mPath);
      else
        mListener.moveFilesFailed(result);

      updateExternalStorages();
    }
  }

  private static void addStoragePathWithSize(String path, Map<Long, String> sizesPaths)
  {
    Log.i(TAG, "Trying to add path " + path);
    try
    {
      final File f = new File(path + "/");
      if (f.exists() && f.isDirectory() && f.canWrite() && isDirWritable(path))
      {
        final long size = getFreeBytesAtPath(path);

        if (size > 0 && !sizesPaths.containsKey(size))
        {
          Log.i(TAG, "Path added: " + path + ", size = " + size);
          sizesPaths.put(size, path);
        }
      }
    } catch (final IllegalArgumentException ex)
    {
      // Suppress exceptions for unavailable storages.
      Log.i(TAG, "StatFs error for path: " + path);
    }
  }

  // http://stackoverflow.com/questions/8151779/find-sd-card-volume-label-on-android
  // http://stackoverflow.com/questions/5694933/find-an-external-sd-card-location
  // http://stackoverflow.com/questions/14212969/file-canwrite-returns-false-on-some-devices-although-write-external-storage-pe
  private static void parseMountFile(String file, int mode, ArrayList<String> paths)
  {
    Log.i(TAG, "Parsing " + file);

    BufferedReader reader = null;
    try
    {
      reader = new BufferedReader(new FileReader(file));

      while (true)
      {
        final String line = reader.readLine();
        if (line == null)
          break;

        // standard regexp for all possible whitespaces (space, tab, etc)
        final String[] arr = line.split("\\s+");

        // split may return empty first strings
        int start = 0;
        while (start < arr.length && arr[start].length() == 0)
          ++start;

        if (arr.length - start > 3)
        {
          if (arr[start].charAt(0) == '#')
            continue;

          if (mode == VOLD_MODE)
          {
            if (arr[start].startsWith("dev_mount"))
              paths.add(arr[start + 2]);
          }
          else
          {
            assert (mode == MOUNTS_MODE);
            final String prefixes[] = {"tmpfs", "/dev/block/vold", "/dev/fuse", "/mnt/media_rw"};
            for (final String s : prefixes)
              if (arr[start].startsWith(s))
                paths.add(arr[start + 1]);
          }
        }
      }
    } catch (final IOException e)
    {
      Log.w(TAG, "Can't read file: " + file);
    } finally
    {
      Utils.closeStream(reader);
    }
  }

  @SuppressWarnings("deprecation")
  private static long getFreeBytesAtPath(String path)
  {
    long size = 0;
    try
    {
      if (Build.VERSION.SDK_INT < android.os.Build.VERSION_CODES.GINGERBREAD)
      {
        final StatFs stat = new StatFs(path);
        size = stat.getAvailableBlocks() * (long) stat.getBlockSize();
      }
      else
        size = new File(path).getFreeSpace();
    } catch (RuntimeException e)
    {
      e.printStackTrace();
    }

    return size;
  }

  // we can't only call canWrite, because on KitKat (Samsung S4) this return
  // true for some directories on sdcard but actually it's read only
  // see https://code.google.com/p/android/issues/detail?id=66369 for details
  private static boolean isDirWritable(String path)
  {
    final File f = new File(path + "/testDir");
    f.mkdir();
    if (f.exists())
    {
      f.delete();
      return true;
    }

    return false;
  }

  private static String getItemFullPath(StoragePathAdapter.StorageItem item)
  {
    return item.mPath + Constants.MWM_DIR_POSTFIX;
  }

  private static String getWritableDirRoot()
  {
    String writableDir = Framework.nativeGetWritableDir();
    int index = writableDir.lastIndexOf(Constants.MWM_DIR_POSTFIX);
    if (index != -1)
      writableDir = writableDir.substring(0, index);

    return writableDir;
  }

  private static native String nativeGenerateUniqueBookmarkName(String baseName);
}
