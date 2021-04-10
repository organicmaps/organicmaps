package com.mapswithme.maps.settings;

import com.mapswithme.maps.BuildConfig;
import com.mapswithme.maps.Framework;
import com.mapswithme.util.Constants;
import com.mapswithme.util.Utils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.FilenameFilter;
import java.io.IOException;
import java.nio.channels.FileChannel;
import java.util.ArrayList;

final class StorageUtils
{
  private StorageUtils() {}

  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.STORAGE);
  private static final String TAG = StorageUtils.class.getSimpleName();

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
