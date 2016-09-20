package com.mapswithme.util;

import android.content.Context;
import android.os.Environment;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.SimpleLogger;

import java.io.File;

public class StorageUtils
{
  private final static Logger LOGGER = SimpleLogger.get(StorageUtils.class.getSimpleName());

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
   * @param context the application context
   * @return the absolute path of external files directory or null if directory cannot be gotten
   * @see Context#getExternalFilesDir(String)
   */
  @Nullable
  public static String getExternalFilesDir(final @NonNull Context context)
  {
    if (isExternalStorageWritable())
    {
      File dir = context.getExternalFilesDir(null);
      if (dir != null)
        return dir.getAbsolutePath();
      else
        LOGGER.e("Cannot get the external files directory for some reasons", new Throwable());
    }
    return null;
  }
}
