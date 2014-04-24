package com.mapswithme.util;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.LinkedHashSet;
import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

import com.mapswithme.maps.R;
import com.mapswithme.maps.settings.StoragePathActivity;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.ProgressDialog;
import android.content.Context;
import android.os.AsyncTask;
import android.os.StatFs;
import android.util.Log;

public class StoragePathManager
{
  private static String TAG = "StoragePathManager";
  private static String MWM_DIR_POSTFIX = "/MapsWithMe/";
  
  public static class StorageItem
  {
    public String m_path;
    public long m_size;

    @Override
    public boolean equals(Object o)
    {
      if (o == this) return true;
      if (o == null) return false;
      return m_size == ((StorageItem)o).m_size;
    }

    @Override
    public int hashCode()
    {
      return Long.valueOf(m_size).hashCode();
    }
  }
  
  static public ArrayList<StorageItem> GetStorages(Context context, String currentPath, String defPath)
  {
    ArrayList<String> pathes = new ArrayList<String>();
    
    parseMountFile("/etc/vold.conf", VOLD_MODE, pathes);
    parseMountFile("/etc/vold.fstab", VOLD_MODE, pathes);
    parseMountFile("/system/etc/vold.fstab", VOLD_MODE, pathes);
    parseMountFile("/proc/mounts", MOUNTS_MODE, pathes);
    
    if (Utils.apiEqualOrGreaterThan(android.os.Build.VERSION_CODES.KITKAT))
    {
      File[] files = context.getExternalFilesDirs(null);
      if (files != null)
      {
        File primaryStorageDir = context.getExternalFilesDir(null);
        for(File f : files)
        {
          if (!f.equals(primaryStorageDir))
            pathes.add(f.getPath());
        }
      }
    }
    
    pathes.add(currentPath);
    pathes.add(defPath);
    
    ArrayList<StorageItem> items = new ArrayList<StorageItem>();
    for (String path : pathes)
      addStorage(path, items);
    
    return new ArrayList<StorageItem>(new LinkedHashSet<StorageItem>(items));
  }
  
  static public String getFullPath(StorageItem item)
  {
    return item.m_path + MWM_DIR_POSTFIX;
  }
  
  /// @name Assume that MapsWithMe folder doesn't have inner folders and symbolic links.
  //@{
  static public long getDirSize(String basePath)
  {
    return getDirSizeImpl(basePath + MWM_DIR_POSTFIX);
  }
  
  static private long getDirSizeImpl(String path)
  {
    final File dir = new File(path);
    assert(dir.exists());
    assert(dir.isDirectory());

    long size = 0;
    for (final File f : dir.listFiles())
    {
      assert(f.isFile());
      size += f.length();
    }

    return (size + 1024*1024);
  }
  
  static public boolean MoveBookmarks()
  {
    ArrayList<String> pathes = new ArrayList<String>();
    if (Utils.apiEqualOrGreaterThan(android.os.Build.VERSION_CODES.KITKAT))
    {
      parseMountFile("/etc/vold.conf", VOLD_MODE, pathes);
      parseMountFile("/etc/vold.fstab", VOLD_MODE, pathes);
      parseMountFile("/system/etc/vold.fstab", VOLD_MODE, pathes);
      parseMountFile("/proc/mounts", MOUNTS_MODE, pathes);
    }
    
    ArrayList<String> approvedPathes = new ArrayList<String>();
    for (String path : pathes)
    {
      String mwmPath = path + MWM_DIR_POSTFIX;
      File f = new File(mwmPath);
      if (f.exists() || f.canRead() || f.isDirectory())
        approvedPathes.add(mwmPath);
    }
    String tmp[] = approvedPathes.toArray(new String[approvedPathes.size()]);
    return nativeMoveBookmarks(tmp, getAvailablePath(nativeGetBookmarkDir()));
  }
  
  static public boolean CheckWritableDir(Context context, SetStoragePathListener listener)
  {
    if (Utils.apiLowerThan(android.os.Build.VERSION_CODES.KITKAT))
      return true;
    
    String settingsDir = nativeGetSettingsDir();
    String writableDir = nativeGetWritableDir();
    
    if (settingsDir == writableDir)
      return true;
    
    File f = new File(writableDir + "testDir");
    f.mkdir();
    if (f.exists())
    {
      // this path is writable. Don't try copy maps
      f.delete();
      return true;
    }

    ArrayList<StorageItem> items = GetStorages(context, writableDir.replace(MWM_DIR_POSTFIX, ""),
        settingsDir.replace(MWM_DIR_POSTFIX, ""));
    long size = getDirSizeImpl(writableDir);
    for (StorageItem item : items)
    {
      if (item.m_size > size)
      {
        SetStoragePathImpl(context, listener, item, null, R.string.kitkat_optimization_in_progress);
        return true;
      }
    }
    return false;
  }
  
  public interface SetStoragePathListener
  {
    void MoveFilesFinished(String newPath);
  }
  
  static public void SetStoragePath(Context context, SetStoragePathListener listener, StorageItem newStorage, StorageItem oldStorage)
  {
    SetStoragePathImpl(context, listener, newStorage, oldStorage, R.string.wait_several_minutes);
  }
  
  static private void SetStoragePathImpl(Context context, SetStoragePathListener listener,
                                         StorageItem newStorage, StorageItem oldStorage,
                                         int messageId)
  {
    MoveFilesTask task = new MoveFilesTask(context, listener, newStorage, oldStorage, messageId);
    task.execute("");
  }
  
  private static int VOLD_MODE = 1;
  private static int MOUNTS_MODE = 2;
  
  // http://stackoverflow.com/questions/8151779/find-sd-card-volume-label-on-android
  // http://stackoverflow.com/questions/5694933/find-an-external-sd-card-location
  // http://stackoverflow.com/questions/14212969/file-canwrite-returns-false-on-some-devices-although-write-external-storage-pe
  static private void parseMountFile(String file, int mode, ArrayList<String> pathes)
  {
    Log.i(TAG, "Parsing " + file);

    BufferedReader reader = null;
    try
    {
      reader = new BufferedReader(new FileReader(file));

      while (true)
      {
        final String line = reader.readLine();
        if (line == null) break;

        // standard regexp for all possible whitespaces (space, tab, etc)
        final String[] arr = line.split("\\s+");

        // split may return empty first strings
        int start = 0;
        while (start < arr.length && arr[start].length() == 0)
          ++start;

        if (arr.length - start > 3)
        {
          if (arr[start + 0].charAt(0) == '#')
            continue;

          if (mode == VOLD_MODE)
          {
            Log.i(TAG, "Label = " + arr[start + 1] + "; Path = " + arr[start + 2]);

            if (arr[start + 0].startsWith("dev_mount"))
              pathes.add(arr[start + 2]);
          }
          else
          {
            assert(mode == MOUNTS_MODE);
            Log.i(TAG, "Label = " + arr[start + 0] + "; Path = " + arr[start + 1]);

            final String prefixes[] = { "tmpfs", "/dev/block/vold", "/dev/fuse", "/mnt/media_rw" };
            for (final String s : prefixes)
              if (arr[start + 0].startsWith(s))
                pathes.add(arr[start + 1]);
          }
        }
      }
    }
    catch (final IOException e)
    {
      Log.w(TAG, "Can't read file: " + file);
    }
    finally
    {
      Utils.closeStream(reader);
    }
  }
  
  private static boolean addStorage(String path, ArrayList<StorageItem> items)
  {
    try
    {
      final File f = new File(path + "/");
      if (f.exists() && f.isDirectory() && f.canWrite())
      {
        // we can't only call canWrite, because on KitKat (Samsung S4) this return true
        // for sdcard but actually it's read only
        File ff = new File(path + "/" + "TestDir");
        ff.mkdir();
        if (!ff.exists())
          return false;
        else 
          ff.delete();
        for (StorageItem item : items)
        {
          if (item.m_path.equals(path))
            return true;
        }

        final long size = getAvailablePath(path);
        Log.i(TAG, "Available size = " + size);

        final StorageItem item = new StorageItem();
        item.m_path = path;
        item.m_size = size;

        items.add(item);
        return true;
      }
      else
        Log.i(TAG, "File error for storage: " + path);
    }
    catch (final IllegalArgumentException ex)
    {
      // Suppress exceptions for unavailable storages.
      Log.i(TAG, "StatFs error for storage: " + path);
    }

    return false;
  }
  
  @SuppressWarnings("deprecation")
  @SuppressLint("NewApi")
  static private long getAvailablePath(String path)
  {
    final StatFs stat = new StatFs(path);
    final long size = Utils.apiLowerThan(android.os.Build.VERSION_CODES.JELLY_BEAN_MR2)
                      ? (long)stat.getAvailableBlocks() * (long)stat.getBlockSize()
                      : stat.getAvailableBytes();
    return size;
  }
  
  static private boolean doMoveMaps(StorageItem newStorage, StorageItem oldStorage)
  {
    String fullNewPath = getFullPath(newStorage);
    File f = new File(fullNewPath);
    if (!f.exists())
      f.mkdir();
    
    assert(f.canWrite());
    assert(f.isDirectory());
    
    if (StoragePathManager.nativeSetStoragePath(fullNewPath))
    {
      if (oldStorage != null)
        deleteFiles(new File(getFullPath(oldStorage)));

      return true;
    }

    return false;
  }
  
  //delete all files (except settings.ini) in directory and bookmarks
  static private void deleteFiles(File dir)
  {
    assert(dir.exists());
    assert(dir.isDirectory());

    for (final File file : dir.listFiles())
    {
      assert(file.isFile());

      // skip settings.ini - this file should be always in one place
      if (file.getName().equalsIgnoreCase("settings.ini"))
        continue;
      
      // skip bookmarks
      if (file.getName().endsWith("kml"))
        continue;

      if (!file.delete())
        Log.w(TAG, "Can't delete file: " + file.getName());
    }
  }
  //@}
  
  private static class MoveFilesTask extends AsyncTask<String, Void, Boolean>
  {
    private final ProgressDialog m_dlg;
    private final StorageItem m_newStorage;
    private final StorageItem m_oldStorage;
    private final SetStoragePathListener m_listener;

    public MoveFilesTask(Context context, SetStoragePathListener listener,
                         StorageItem newStorage, StorageItem oldStorage, int messageID)
    {
      m_newStorage = newStorage;
      m_oldStorage = oldStorage;
      m_listener = listener;
      
      m_dlg = new ProgressDialog(context);
      m_dlg.setMessage(context.getString(messageID));
      m_dlg.setProgressStyle(ProgressDialog.STYLE_SPINNER);
      m_dlg.setIndeterminate(true);
      m_dlg.setCancelable(false);
    }

    @Override
    protected void onPreExecute()
    {
      m_dlg.show();
    }

    @Override
    protected Boolean doInBackground(String... params)
    {
      return doMoveMaps(m_newStorage, m_oldStorage);
    }

    @Override
    protected void onPostExecute(Boolean result)
    {
      // Using dummy try-catch because of the following:
      // http://stackoverflow.com/questions/2745061/java-lang-illegalargumentexception-view-not-attached-to-window-manager
      try
      {
        m_dlg.dismiss();
      }
      catch (final Exception e)
      {
      }

      if (result)
        m_listener.MoveFilesFinished(m_newStorage.m_path);
    }
  }
  
  static private native boolean nativeMoveBookmarks(String[] additionalFindPathes, long availableSize);
  static private native boolean nativeSetStoragePath(String newPath);
  static private native String nativeGetBookmarkDir();
  static private native String nativeGetSettingsDir();
  static private native String nativeGetWritableDir();
}
