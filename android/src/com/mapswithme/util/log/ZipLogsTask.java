package com.mapswithme.util.log;

import android.support.annotation.NonNull;
import android.util.Log;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.concurrent.Callable;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;


class ZipLogsTask implements Callable<Boolean>
{
  private final static String TAG = ZipLogsTask.class.getSimpleName();
  @NonNull
  private final String mSourcePath;
  @NonNull
  private final String mDestPath;

  ZipLogsTask(@NonNull String sourcePath, @NonNull String destPath)
  {
    mSourcePath = sourcePath;
    mDestPath = destPath;
  }

  @Override
  public Boolean call() throws Exception
  {
    return zipFileAtPath(mSourcePath, mDestPath);
  }

  private boolean zipFileAtPath(@NonNull String sourcePath, @NonNull String toLocation)
  {
    final int buffer = 2048;

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
        byte data[] = new byte[buffer];
        FileInputStream fi = new FileInputStream(sourcePath);
        origin = new BufferedInputStream(fi, buffer);
        ZipEntry entry = new ZipEntry(getLastPathComponent(sourcePath));
        out.putNextEntry(entry);
        int count;
        while ((count = origin.read(data, 0, buffer)) != -1)
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
    final int buffer = 2048;

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
        byte data[] = new byte[buffer];
        String unmodifiedFilePath = file.getPath();
        String relativePath = unmodifiedFilePath
            .substring(basePathLength);
        FileInputStream fi = new FileInputStream(unmodifiedFilePath);
        origin = new BufferedInputStream(fi, buffer);
        ZipEntry entry = new ZipEntry(relativePath);
        out.putNextEntry(entry);
        int count;
        while ((count = origin.read(data, 0, buffer)) != -1)
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
}
