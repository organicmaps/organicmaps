package com.mapswithme.util.log;

import android.app.Application;
import android.text.TextUtils;
import android.util.Log;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

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
  private final static Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
  private final static int BUFFER_SIZE = 2048;
  @NonNull
  private final String mLogsPath;
  @NonNull
  private final String mZipPath;
  @Nullable
  private final LoggerFactory.OnZipCompletedListener mOnCompletedListener;

  ZipLogsTask(@NonNull String logsPath, @NonNull String zipPath,
              @NonNull LoggerFactory.OnZipCompletedListener onCompletedListener)
  {
    mLogsPath = logsPath;
    mZipPath = zipPath;
    mOnCompletedListener = onCompletedListener;
  }

  @Override
  public void run()
  {
    saveSystemLogcat(mLogsPath);
    final boolean success = zipFileAtPath(mLogsPath, mZipPath);
    if (mOnCompletedListener != null)
      mOnCompletedListener.onCompleted(success, mZipPath);
  }

  private boolean zipFileAtPath(@NonNull String sourcePath, @NonNull String toLocation)
  {
    File sourceFile = new File(sourcePath);
    if (!sourceFile.isDirectory())
      return false;
    try (
        FileOutputStream dest = new FileOutputStream(toLocation, false);
        ZipOutputStream out = new ZipOutputStream(new BufferedOutputStream(dest)))
    {
      zipSubFolder(out, sourceFile, sourceFile.getParent().length());
    }
    catch (Exception e)
    {
      Log.e(TAG, "Failed to zip file '" + sourcePath + "' to location '" + toLocation + "'", e);
      return false;
    }
    return true;
  }

  private void zipSubFolder(ZipOutputStream out, File folder,
                            int basePathLength) throws IOException
  {
    File[] fileList = folder.listFiles();
    if (fileList == null)
      throw new IOException("Can't get files list of " + folder.getPath());

    for (File file : fileList)
    {
      if (file.isDirectory())
      {
        zipSubFolder(out, file, basePathLength);
      }
      else
      {
        byte[] data = new byte[BUFFER_SIZE];
        String unmodifiedFilePath = file.getPath();
        String relativePath = unmodifiedFilePath.substring(basePathLength);
        try (
            FileInputStream fi = new FileInputStream(unmodifiedFilePath);
            BufferedInputStream origin = new BufferedInputStream(fi, BUFFER_SIZE))
        {
          ZipEntry entry = new ZipEntry(relativePath);
          out.putNextEntry(entry);
          int count;
          while ((count = origin.read(data, 0, BUFFER_SIZE)) != -1)
          {
            out.write(data, 0, count);
          }
        }
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

  private void saveSystemLogcat(String path)
  {
    final String cmd = "logcat -d -v time";
    Process process;
    try
    {
      process = Runtime.getRuntime().exec(cmd);
    }
    catch (IOException e)
    {
      LOGGER.e(TAG, "Failed to get system logcat", e);
      return;
    }

    path += File.separator + "logcat.log";
    final File file = new File(path);
    try (
        InputStreamReader reader = new InputStreamReader(process.getInputStream());
        FileWriter writer = new FileWriter(file))
    {
      LoggerFactory.INSTANCE.writeSystemInformation(writer);
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
      LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
      LOGGER.e(TAG, "Failed to save system logcat to " + path, e);
    }
  }
}
