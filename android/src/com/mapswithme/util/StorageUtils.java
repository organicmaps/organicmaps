package com.mapswithme.util;

import android.content.Context;
import android.os.Environment;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.text.TextUtils;
import android.util.Log;

import com.mapswithme.maps.MwmApplication;

import java.io.File;

public class StorageUtils
{
  private final static String LOGS_FOLDER = "logs";

  /**
   * Checks if external storage is available for read and write
   *
   * @return true if external storage is mounted and ready for reading/writing
   */
  private static boolean isExternalStorageWritable()
  {
    String state = Environment.getExternalStorageState();
    return Environment.MEDIA_MOUNTED.equals(state);
  }

  /**
   * Safely returns the external files directory path with the preliminary
   * checking the availability of the mentioned directory
   *
   * @return the absolute path of external files directory or null if directory can not be obtained
   * @see Context#getExternalFilesDir(String)
   */
  @Nullable
  private static String getExternalFilesDir()
  {
    if (!isExternalStorageWritable())
      return null;

    File dir = MwmApplication.get().getExternalFilesDir(null);
    if (dir != null)
      return dir.getAbsolutePath();

    Log.e(StorageUtils.class.getSimpleName(),
          "Cannot get the external files directory for some reasons", new Throwable());
    return null;
  }

  /**
   * Check existence of the folder for writing the logs. If that folder is absent this method will
   * try to create it and all missed parent folders.
   * @return true - if folder exists, otherwise - false
   */
  public static boolean ensureLogsFolderExistence()
  {
    String externalDir = StorageUtils.getExternalFilesDir();
    if (TextUtils.isEmpty(externalDir))
      return false;

    File folder = new File(externalDir + File.separator + LOGS_FOLDER);
    boolean success = true;
    if (!folder.exists())
      success = folder.mkdirs();
    return success;
  }

  @Nullable
  public static String getLogsFolder()
  {
    if (!ensureLogsFolderExistence())
      return null;

    String externalDir = StorageUtils.getExternalFilesDir();
    return externalDir + File.separator + LOGS_FOLDER;
  }

  @Nullable
  static String getLogsZipPath()
  {
    String zipFile = getExternalFilesDir() + File.separator + LOGS_FOLDER + ".zip";
    File file = new File(zipFile);
    return file.isFile() && file.exists() ? zipFile : null;
  }

  public static long getFileSize(@NonNull String path)
  {
    File file = new File(path);
    return file.length();
  }
}
