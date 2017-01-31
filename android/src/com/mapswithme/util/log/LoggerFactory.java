package com.mapswithme.util.log;

import android.content.SharedPreferences;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.text.TextUtils;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.util.StorageUtils;
import net.jcip.annotations.GuardedBy;
import net.jcip.annotations.ThreadSafe;

import java.io.File;
import java.util.EnumMap;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

@ThreadSafe
public class LoggerFactory
{
  public enum Type
  {
    MISC, LOCATION, TRAFFIC, GPS_TRACKING, TRACK_RECORDER, ROUTING, NETWORK, STORAGE, DOWNLOADER
  }

  public interface OnZipCompletedListener
  {
    /**
     * Indicates about completion of zipping operation.
     * <p>
     * <b>NOTE:</b> called from the logger thread
     * </p>
     * @param success indicates about a status of zipping operation
     */
    void onCompleted(boolean success);
  }

  public final static LoggerFactory INSTANCE = new LoggerFactory();
  private final static String PREF_FILE_LOGGING_ENABLED = "FileLoggingEnabled";

  @NonNull
  @GuardedBy("this")
  private final EnumMap<Type, BaseLogger> mLoggers = new EnumMap<>(Type.class);
  @Nullable
  @GuardedBy("this")
  private ExecutorService mFileLoggerExecutor;

  private LoggerFactory()
  {
  }

  public boolean isFileLoggingEnabled()
  {
    SharedPreferences prefs = MwmApplication.prefs();
    return prefs.getBoolean(PREF_FILE_LOGGING_ENABLED, false);
  }

  public void setFileLoggingEnabled(boolean enabled)
  {
    if (isFileLoggingEnabled() == enabled)
      return;

    SharedPreferences prefs = MwmApplication.prefs();
    SharedPreferences.Editor editor = prefs.edit();
    editor.putBoolean(PREF_FILE_LOGGING_ENABLED, enabled).apply();
    updateLoggers();
  }

  @NonNull
  public synchronized Logger getLogger(@NonNull Type type)
  {
    BaseLogger logger = mLoggers.get(type);
    if (logger == null)
    {
      logger = createLogger(type);
      mLoggers.put(type, logger);
    }
    return logger;
  }

  private synchronized void updateLoggers()
  {
    for (Type type: mLoggers.keySet())
    {
      BaseLogger logger = mLoggers.get(type);
      logger.setStrategy(createLoggerStrategy(type));
    }
  }

  public synchronized void zipLogs(@Nullable OnZipCompletedListener listener)
  {
    if (!isFileLoggingEnabled())
    {
      if (listener != null)
        listener.onCompleted(false);
      return;
    }

    String logsFolder = StorageUtils.getLogsFolder();

    if (TextUtils.isEmpty(logsFolder))
    {
      if (listener != null)
        listener.onCompleted(false);
      return;
    }

    Runnable task = new ZipLogsTask(logsFolder, logsFolder + ".zip", listener);
    getFileLoggerExecutor().execute(task);
  }

  @NonNull
  private BaseLogger createLogger(@NonNull Type type)
  {
    LoggerStrategy strategy = createLoggerStrategy(type);
    return new BaseLogger(strategy);
  }

  @NonNull
  private LoggerStrategy createLoggerStrategy(@NonNull Type type)
  {
    SharedPreferences prefs = MwmApplication.prefs();
    if (prefs.getBoolean(PREF_FILE_LOGGING_ENABLED, false))
    {
      String logsFolder = StorageUtils.getLogsFolder();
      if (!TextUtils.isEmpty(logsFolder))
        return new FileLoggerStrategy(logsFolder + File.separator
                                      + type.name().toLowerCase() + ".log", getFileLoggerExecutor());
    }

    return new LogCatStrategy();
  }

  @NonNull
  private synchronized ExecutorService getFileLoggerExecutor()
  {
    if (mFileLoggerExecutor == null)
      mFileLoggerExecutor = Executors.newSingleThreadExecutor();
    return mFileLoggerExecutor;
  }
}
