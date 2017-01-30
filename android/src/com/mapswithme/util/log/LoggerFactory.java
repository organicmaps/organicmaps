package com.mapswithme.util.log;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.text.TextUtils;
import android.util.Log;

import com.mapswithme.util.Config;
import com.mapswithme.util.StorageUtils;
import net.jcip.annotations.GuardedBy;
import net.jcip.annotations.ThreadSafe;

import java.io.File;
import java.util.EnumMap;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;

@ThreadSafe
public class LoggerFactory
{
  public enum Type
  {
    MISC, LOCATION, TRAFFIC, GPS_TRACKING, TRACK_RECORDER, ROUTING, NETWORK;
  }

  public interface OnZipCompletedListener
  {
    /**
     * Indicates about completion of zipping operation.
     * <p>
     * <b>NOTE:</b> called from the logger thread
     * </p>
     * @param success indicates a status of zipping operation
     */
    void onComplete(boolean success);
  }

  public final static LoggerFactory INSTANCE = new LoggerFactory();
  private final static String TAG = LoggerFactory.class.getSimpleName();

  @NonNull
  @GuardedBy("this")
  private final EnumMap<Type, BaseLogger> mLoggers = new EnumMap<>(Type.class);
  @Nullable
  @GuardedBy("this")
  private ExecutorService mFileLoggerExecutor;

  private LoggerFactory()
  {
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

  public synchronized void updateLoggers()
  {
    for (Type type: mLoggers.keySet())
    {
      BaseLogger logger = mLoggers.get(type);
      logger.setStrategy(createLoggerStrategy(type));
    }
  }

  public synchronized void zipLogs(@Nullable OnZipCompletedListener listener)
  {
    if (!Config.isLoggingEnabled())
    {
      if (listener != null)
        listener.onComplete(false);
      return;
    }

    String logsFolder = StorageUtils.getLogsFolder();

    if (TextUtils.isEmpty(logsFolder))
    {
      if (listener != null)
        listener.onComplete(false);
      return;
    }

    Callable<Boolean> task = new ZipLogsTask(logsFolder, logsFolder + ".zip");
    Future<Boolean> result = getFileLoggerExecutor().submit(task);
    boolean success = false;
    try
    {
      success = result.get();
    }
    catch (InterruptedException | ExecutionException e)
    {
      Log.e(TAG, "Failed to zip logs: ", e);
    }

    if (listener != null)
      listener.onComplete(success);
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
    if (Config.isLoggingEnabled())
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
