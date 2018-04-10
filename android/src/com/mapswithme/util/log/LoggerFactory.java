package com.mapswithme.util.log;

import android.content.SharedPreferences;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.text.TextUtils;
import android.util.Log;

import com.mapswithme.maps.BuildConfig;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
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
    MISC, LOCATION, TRAFFIC, GPS_TRACKING, TRACK_RECORDER, ROUTING, NETWORK, STORAGE, DOWNLOADER,
    CORE
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
  @NonNull
  @GuardedBy("this")
  private final EnumMap<Type, BaseLogger> mLoggers = new EnumMap<>(Type.class);
  private final static String CORE_TAG = "MapsmeCore";
  @Nullable
  @GuardedBy("this")
  private ExecutorService mFileLoggerExecutor;

  private LoggerFactory()
  {
  }

  public boolean isFileLoggingEnabled()
  {
    SharedPreferences prefs = MwmApplication.prefs();
    String enableLoggingKey = MwmApplication.get().getString(R.string.pref_enable_logging);
    return prefs.getBoolean(enableLoggingKey, BuildConfig.BUILD_TYPE.equals("beta"));
  }

  public void setFileLoggingEnabled(boolean enabled)
  {
    nativeToggleCoreDebugLogs(enabled);
    SharedPreferences prefs = MwmApplication.prefs();
    SharedPreferences.Editor editor = prefs.edit();
    String enableLoggingKey = MwmApplication.get().getString(R.string.pref_enable_logging);
    editor.putBoolean(enableLoggingKey, enabled).apply();
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
    if (isFileLoggingEnabled())
    {
      nativeToggleCoreDebugLogs(true);
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

  // Called from JNI.
  @SuppressWarnings("unused")
  private static void logCoreMessage(int level, String msg)
  {
    Logger logger = INSTANCE.getLogger(Type.CORE);
    switch (level)
    {
      case Log.DEBUG:
        logger.d(CORE_TAG, msg);
        break;
      case Log.INFO:
        logger.i(CORE_TAG, msg);
        break;
      case Log.WARN:
        logger.w(CORE_TAG, msg);
        break;
      case Log.ERROR:
        logger.e(CORE_TAG, msg);
        break;
      default:
        logger.v(CORE_TAG, msg);
    }
  }

  private static native void nativeToggleCoreDebugLogs(boolean enabled);
}
