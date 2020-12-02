package com.mapswithme.maps.settings;

import android.annotation.TargetApi;
import android.os.Build;
import android.text.TextUtils;

import com.mapswithme.maps.BuildConfig;
import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.util.Constants;
import com.mapswithme.util.Utils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.FilenameFilter;
import java.io.IOException;
import java.nio.channels.FileChannel;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.Set;

final class StorageUtils
{
  private StorageUtils() {}

  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.STORAGE);
  private static final String TAG = StorageUtils.class.getSimpleName();
  private static final int VOLD_MODE = 1;
  private static final int MOUNTS_MODE = 2;

  /**
   * Check if directory is writable. On some devices with KitKat (eg, Samsung S4) simple File.canWrite() returns
   * true for some actually read only directories on sdcard.
   * see https://code.google.com/p/android/issues/detail?id=66369 for details
   *
   * @param path path to ckeck
   * @return result
   */
  @SuppressWarnings("ResultOfMethodCallIgnored")
  static boolean isDirWritable(String path)
  {
    File f = new File(path, "mapsme_test_dir");
    f.mkdir();
    if (!f.exists())
      return false;

    f.delete();
    return true;
  }

  /**
   * Returns path, where maps and other files are stored.
   * @return pat (or empty string, if framework wasn't created yet)
   */
  static String getWritableDirRoot()
  {
    String writableDir = Framework.nativeGetWritableDir();
    int index = writableDir.lastIndexOf(Constants.MWM_DIR_POSTFIX);
    if (index != -1)
      writableDir = writableDir.substring(0, index);

    return writableDir;
  }

  static long getFreeBytesAtPath(String path)
  {
    long size = 0;
    try
    {
      size = new File(path).getFreeSpace();
    } catch (RuntimeException e)
    {
      e.printStackTrace();
    }

    return size;
  }

  // http://stackoverflow.com/questions/8151779/find-sd-card-volume-label-on-android
  // http://stackoverflow.com/questions/5694933/find-an-external-sd-card-location
  // http://stackoverflow.com/questions/14212969/file-canwrite-returns-false-on-some-devices-although-write-external-storage-pe
  private static void parseMountFile(String file, int mode, Set<String> paths)
  {
    LOGGER.i(StoragePathManager.TAG, "Parsing " + file);

    BufferedReader reader = null;
    try
    {
      reader = new BufferedReader(new FileReader(file));

      while (true)
      {
        String line = reader.readLine();
        if (line == null)
          return;

        line = line.trim();
        if (TextUtils.isEmpty(line) || line.startsWith("#"))
          continue;

        // standard regexp for all possible whitespaces (space, tab, etc)
        String[] parts = line.split("\\s+");

        if (parts.length <= 3)
          continue;

        if (mode == VOLD_MODE)
        {
          if (parts[0].startsWith("dev_mount"))
            paths.add(parts[2]);

          continue;
        }

        for (String s : new String[] { "/dev/block/vold", "/dev/fuse", "/mnt/media_rw" })
        {
          if (parts[0].startsWith(s))
          {
            paths.add(parts[1]);
            break;
          }
        }
      }
    } catch (final IOException e)
    {
      LOGGER.w(TAG, "Can't read file: " + file, e);
    } finally
    {
      Utils.closeSafely(reader);
    }
  }

  static void parseStorages(Set<String> paths)
  {
    parseMountFile("/etc/vold.conf", VOLD_MODE, paths);
    parseMountFile("/etc/vold.fstab", VOLD_MODE, paths);
    parseMountFile("/system/etc/vold.fstab", VOLD_MODE, paths);
    parseMountFile("/proc/mounts", MOUNTS_MODE, paths);
  }

  static void copyFile(File source, File dest) throws IOException
  {
    int maxChunkSize = 10 * Constants.MB; // move file by smaller chunks to avoid OOM.
    FileChannel inputChannel = null, outputChannel = null;
    try
    {
      inputChannel = new FileInputStream(source).getChannel();
      outputChannel = new FileOutputStream(dest).getChannel();
      long totalSize = inputChannel.size();

      for (long currentPosition = 0; currentPosition < totalSize; currentPosition += maxChunkSize)
      {
        outputChannel.position(currentPosition);
        outputChannel.transferFrom(inputChannel, currentPosition, maxChunkSize);
      }
    } finally
    {
      Utils.closeSafely(inputChannel);
      Utils.closeSafely(outputChannel);
    }
  }

  private static long getDirSizeRecursively(File file, FilenameFilter fileFilter)
  {
    if (file.isDirectory())
    {
      long dirSize = 0;
      for (File child : file.listFiles())
        dirSize += getDirSizeRecursively(child, fileFilter);

      return dirSize;
    }

    if (fileFilter.accept(file.getParentFile(), file.getName()))
      return file.length();

    return 0;
  }

  static long getWritableDirSize()
  {
    final File writableDir = new File(Framework.nativeGetWritableDir());
    if (BuildConfig.DEBUG)
    {
      if (!writableDir.exists())
        throw new IllegalStateException("Writable directory doesn't exits, can't get size.");
      if (!writableDir.isDirectory())
        throw new IllegalStateException("Writable directory isn't a directory, can't get size.");
    }

    return getDirSizeRecursively(writableDir, StoragePathManager.MOVABLE_FILES_FILTER);
  }

  /**
   * Recursively lists all movable files in the directory.
   */
  static void listFilesRecursively(File dir, String prefix, FilenameFilter filter, ArrayList<String> relPaths)
  {
    File[] list = dir.listFiles();
    if (list == null)
      return;

    for (File file : list)
    {
      if (file.isDirectory())
      {
        listFilesRecursively(file, prefix + file.getName() + File.separator, filter, relPaths);
        continue;
      }
      String name = file.getName();
      if (filter.accept(dir, name))
        relPaths.add(prefix + name);
    }
  }

  @SuppressWarnings("ResultOfMethodCallIgnored")
  private static void removeEmptyDirectories(File dir)
  {
    for (File file : dir.listFiles())
    {
      if (!file.isDirectory())
        continue;
      removeEmptyDirectories(file);
      file.delete();
    }
  }

  @SuppressWarnings("ResultOfMethodCallIgnored")
  static boolean removeFilesInDirectory(File dir, File[] files)
  {
    try
    {
      for (File file : files)
      {
        if (file != null)
          file.delete();
      }
      removeEmptyDirectories(dir);
      return true;
    } catch (Exception e)
    {
      e.printStackTrace();
      return false;
    }
  }

}
