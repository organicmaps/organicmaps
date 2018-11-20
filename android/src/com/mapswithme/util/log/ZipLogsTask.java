package com.mapswithme.util.log;

import android.app.Application;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.util.Log;

import com.mapswithme.util.StorageUtils;
import com.mapswithme.util.Utils;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;


class ZipLogsTask implements Runnable
{
  private final static String TAG = ZipLogsTask.class.getSimpleName();
  private final static int BUFFER_SIZE = 2048;
  @NonNull
  private final String mSourcePath;
  @NonNull
  private final String mDestPath;
  @Nullable
  private final LoggerFactory.OnZipCompletedListener mOnCompletedListener;
  @NonNull
  private final Application mApplication;

  ZipLogsTask(@NonNull Application application, @NonNull String sourcePath,
              @NonNull String destPath,
              @Nullable LoggerFactory.OnZipCompletedListener onCompletedListener)
  {
    mApplication = application;
    mSourcePath = sourcePath;
    mDestPath = destPath;
    mOnCompletedListener = onCompletedListener;
  }

  @Override
  public void run()
  {
    saveSystemLogcat();
    boolean success = zipFileAtPath(mSourcePath, mDestPath);
    if (mOnCompletedListener != null)
      mOnCompletedListener.onCompleted(success);
  }

  private boolean zipFileAtPath(@NonNull String sourcePath, @NonNull String toLocation)
  {
    File sourceFile = new File(sourcePath);
    try
    {
      BufferedInputStream origin;
      FileOutputStream dest = new FileOutputStream(toLocation, false);
      ZipOutputStream out = new ZipOutputStream(new BufferedOutputStream(dest));
      if (sourceFile.isDirectory())
      {
        zipSubFolder(out, sourceFile, sourceFile.getParent().length());
      }
      else
      {
        byte data[] = new byte[BUFFER_SIZE];
        FileInputStream fi = new FileInputStream(sourcePath);
        origin = new BufferedInputStream(fi, BUFFER_SIZE);
        ZipEntry entry = new ZipEntry(getLastPathComponent(sourcePath));
        out.putNextEntry(entry);
        int count;
        while ((count = origin.read(data, 0, BUFFER_SIZE)) != -1)
        {
          out.write(data, 0, count);
        }
      }
      out.close();
    }
    catch (Exception e)
    {
      Log.e(TAG, "Failed to zip file '" + sourcePath +"' to location '" + toLocation + "'", e);
      return false;
    }
    return true;
  }

  private void zipSubFolder(ZipOutputStream out, File folder,
                            int basePathLength) throws IOException
  {
    File[] fileList = folder.listFiles();
    BufferedInputStream origin;
    for (File file : fileList)
    {
      if (file.isDirectory())
      {
        zipSubFolder(out, file, basePathLength);
      }
      else
      {
        byte data[] = new byte[BUFFER_SIZE];
        String unmodifiedFilePath = file.getPath();
        String relativePath = unmodifiedFilePath
            .substring(basePathLength);
        FileInputStream fi = new FileInputStream(unmodifiedFilePath);
        origin = new BufferedInputStream(fi, BUFFER_SIZE);
        ZipEntry entry = new ZipEntry(relativePath);
        out.putNextEntry(entry);
        int count;
        while ((count = origin.read(data, 0, BUFFER_SIZE)) != -1)
        {
          out.write(data, 0, count);
        }
        origin.close();
      }
    }
  }

  private static String getLastPathComponent(String filePath)
  {
    String[] segments = filePath.split(File.separator);
    if (segments.length == 0)
      return "";
    return segments[segments.length - 1];
  }

  private void saveSystemLogcat()
  {
    String fullName = StorageUtils.getLogsFolder(mApplication) + File.separator + "logcat.log";
    final File file = new File(fullName);
    InputStreamReader reader = null;
    FileWriter writer = null;
    try
    {
      writer = new FileWriter(file);
      FileLoggerStrategy.WriteTask.writeSystemInformation(mApplication, writer);
      String cmd = "logcat -d -v time";
      Process process = Runtime.getRuntime().exec(cmd);
      reader = new InputStreamReader(process.getInputStream());
      char[] buffer = new char[10000];
      do
      {
        int n = reader.read(buffer, 0, buffer.length);
        if (n == -1)
          break;
        writer.write(buffer, 0, n);
      } while (true);
    }
    catch (Throwable e)
    {
      Log.e(TAG, "Failed to save system logcat to file: " + file, e);
    }
    finally
    {
      Utils.closeSafely(writer);
      Utils.closeSafely(reader);
    }
  }
}
