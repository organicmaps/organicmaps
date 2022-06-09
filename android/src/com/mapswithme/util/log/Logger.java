package com.mapswithme.util.log;

import android.util.Log;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.mapswithme.maps.BuildConfig;
import net.jcip.annotations.ThreadSafe;

import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;

@ThreadSafe
public final class Logger
{
  private static final String TAG = Logger.class.getSimpleName();
  private static final String CORE_TAG = "OMcore";
  private static final String FILENAME = "app.log";

  public static void v(String tag, String msg)
  {
    log(Log.VERBOSE, tag, msg, null);
  }

  public static void v(String tag, String msg, Throwable tr)
  {
    log(Log.VERBOSE, tag, msg, tr);
  }

  public static void d(String tag, String msg)
  {
    log(Log.DEBUG, tag, msg, null);
  }

  public static void d(String tag, String msg, Throwable tr)
  {
    log(Log.DEBUG, tag, msg, tr);
  }

  public static void i(String tag, String msg)
  {
    log(Log.INFO, tag, msg, null);
  }

  public static void i(String tag, String msg, Throwable tr)
  {
    log(Log.INFO, tag, msg, tr);
  }

  public static void w(String tag, String msg)
  {
    log(Log.WARN, tag, msg, null);
  }

  public static void w(String tag, String msg, Throwable tr)
  {
    log(Log.WARN, tag, msg, tr);
  }

  public static void e(String tag, String msg)
  {
    log(Log.ERROR, tag, msg, null);
  }

  public static void e(String tag, String msg, Throwable tr)
  {
    log(Log.ERROR, tag, msg, tr);
  }

  // Called from JNI to proxy native code logging.
  @SuppressWarnings("unused")
  @Keep
  private static void logCoreMessage(int level, String msg)
  {
    log(level, CORE_TAG, msg, null);
  }

  public static void log(int level, @NonNull String tag, @NonNull String msg, @Nullable Throwable tr)
  {
    final String logsFolder = LogsManager.INSTANCE.getEnabledLogsFolder();
    if (logsFolder != null)
    {
      final String data = getLevelChar(level) + "/" + tag + ": " + msg + (tr != null ? '\n' + Log.getStackTraceString(tr) : "");
      LogsManager.EXECUTOR.execute(new WriteTask(logsFolder + File.separator + FILENAME,
                                                 data, Thread.currentThread().getName()));
    }
    else if (BuildConfig.DEBUG || level >= Log.INFO)
    {
      // Only Debug builds log DEBUG level to Android system log.
      if (tr != null)
        msg += '\n' + Log.getStackTraceString(tr);
      Log.println(level, tag, msg);
    }
  }

  private static char getLevelChar(int level)
  {
    switch (level)
    {
      case Log.VERBOSE:
        return 'V';
      case Log.DEBUG:
        return 'D';
      case Log.INFO:
        return 'I';
      case Log.WARN:
        return 'W';
      case Log.ERROR:
        return 'E';
    }
    assert false : "Unknown log level " + level;
    return '_';
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
          fw.write(LogsManager.INSTANCE.getSystemInformation());
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
        Log.e(TAG, "Failed to write to " + mFilePath + ": " + mData, e);
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
