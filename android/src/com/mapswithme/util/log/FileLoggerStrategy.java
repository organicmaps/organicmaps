package com.mapswithme.util.log;

import android.text.TextUtils;
import android.util.Log;

import androidx.annotation.NonNull;
import net.jcip.annotations.Immutable;

import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;
import java.util.concurrent.Executor;

@Immutable
class FileLoggerStrategy implements LoggerStrategy
{
  private static final String TAG = FileLoggerStrategy.class.getSimpleName();
  @NonNull
  private final String mFileName;
  @NonNull
  private final Executor mExecutor;

  public FileLoggerStrategy(@NonNull String fileName, @NonNull Executor executor)
  {
    mFileName = fileName;
    mExecutor = executor;
  }

  @Override
  public void v(String tag, String msg)
  {
    write("V/" + tag + ": " + msg);
  }

  @Override
  public void v(String tag, String msg, Throwable tr)
  {
    write("V/" + tag + ": " + msg + "\n" + Log.getStackTraceString(tr));
  }

  @Override
  public void d(String tag, String msg)
  {
    write("D/" + tag + ": " + msg);
  }

  @Override
  public void d(String tag, String msg, Throwable tr)
  {
    write("D/" + tag + ": " + msg + "\n" + Log.getStackTraceString(tr));
  }

  @Override
  public void i(String tag, String msg)
  {
    write("I/" + tag + ": " + msg);
  }

  @Override
  public void i(String tag, String msg, Throwable tr)
  {

    write("I/" + tag + ": " + msg + "\n" + Log.getStackTraceString(tr));
  }

  @Override
  public void w(String tag, String msg)
  {
    write("W/" + tag + ": " + msg);
  }

  @Override
  public void w(String tag, String msg, Throwable tr)
  {
    write("W/" + tag + ": " + msg + "\n" + Log.getStackTraceString(tr));
  }

  @Override
  public void w(String tag, Throwable tr)
  {
    write("D/" + tag + ": " + Log.getStackTraceString(tr));
  }

  @Override
  public void e(String tag, String msg)
  {
    write("E/" + tag + ": " + msg);
  }

  @Override
  public void e(String tag, String msg, Throwable tr)
  {
    write("E/" + tag + ": " + msg + "\n" + Log.getStackTraceString(tr));
  }

  private void write(@NonNull final String data)
  {
    final String logsPath = LoggerFactory.INSTANCE.ensureLogsFolder();
    if (logsPath == null)
      Log.e(TAG, "Couldn't log to " + mFileName + ": " + data);
    else
      mExecutor.execute(new WriteTask(logsPath + File.separator + mFileName,
                                      data, Thread.currentThread().getName()));
  }

  private static class WriteTask implements Runnable
  {
    private static final int MAX_SIZE = 3000000;
    @NonNull
    private final String mFilePath;
    @NonNull
    private final String mData;
    @NonNull
    private final String mCallingThread;

    private WriteTask(@NonNull String filePath, @NonNull String data, @NonNull String callingThread)
    {
      mFilePath = filePath;
      mData = data;
      mCallingThread = callingThread;
    }

    @Override
    public void run()
    {
      FileWriter fw = null;
      try
      {
        File file = new File(mFilePath);
        if (!file.exists() || file.length() > MAX_SIZE)
        {
          fw = new FileWriter(file, false);
          LoggerFactory.INSTANCE.writeSystemInformation(fw);
        }
        else
        {
          fw = new FileWriter(mFilePath, true);
        }
        DateFormat formatter = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss.SSS", Locale.US);
        fw.write(formatter.format(new Date()) + " " + mCallingThread + ": " + mData + "\n");
      }
      catch (IOException e)
      {
        Log.e(TAG, "Failed to log: " + mData, e);
      }
      finally
      {
        if (fw != null)
          try
          {
            fw.close();
          }
          catch (IOException e)
          {
            Log.e(TAG, "Failed to close file " + mFilePath, e);
          }
      }
    }
  }
}
