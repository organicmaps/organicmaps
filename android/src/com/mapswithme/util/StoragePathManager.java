package com.mapswithme.util;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.LinkedHashSet;

import android.annotation.SuppressLint;
import android.content.Context;
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
    final File dir = new File(basePath + MWM_DIR_POSTFIX);
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
  
  static private native boolean nativeMoveBookmarks(String[] additionalFindPathes, long availableSize);
  static private native String nativeGetBookmarkDir();
}
