package com.mapswithme.util.log;

import android.app.Application;
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
import ru.mail.notify.core.utils.LogReceiver;

import java.io.File;
import java.util.EnumMap;
import java.util.Objects;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

@ThreadSafe
public class LoggerFactory
{
  public enum Type
  {
    MISC, LOCATION, TRAFFIC, GPS_TRACKING, TRACK_RECORDER, ROUTING, NETWORK, STORAGE, DOWNLOADER,
    CORE, THIRD_PARTY, BILLING
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
  @Nullable
  private Application mApplication;

  private LoggerFactory()
  {
  }

  public void initialize(@NonNull Application application)
  {
    mApplication = application;
  }

  public boolean isFileLoggingEnabled()
  {
    if (mApplication == null)
    {
      if (BuildConfig.DEBUG)
        throw new IllegalStateException("Application is not created," +
                                        "but logger is used!");
      return false;
    }

    SharedPreferences prefs = MwmApplication.prefs(mApplication);
    String enableLoggingKey = mApplication.getString(R.string.pref_enable_logging);
    //noinspection ConstantConditions
    return prefs.getBoolean(enableLoggingKey, BuildConfig.BUILD_TYPE.equals("beta"));
  }

  public void setFileLoggingEnabled(boolean enabled)
  {
    Objects.requireNonNull(mApplication);
    nativeToggleCoreDebugLogs(enabled);
    SharedPreferences prefs = MwmApplication.prefs(mApplication);
    SharedPreferences.Editor editor = prefs.edit();
    String enableLoggingKey = mApplication.getString(R.string.pref_enable_logging);
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
    if (mApplication == null)
      return;

    String logsFolder = StorageUtils.getLogsFolder(mApplication);

    if (TextUtils.isEmpty(logsFolder))
    {
      if (listener != null)
        listener.onCompleted(false);
      return;
    }

    Runnable task = new ZipLogsTask(mApplication, logsFolder, logsFolder + ".zip", listener);
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
    if (isFileLoggingEnabled() && mApplication != null)
    {
      nativeToggleCoreDebugLogs(true);
      String logsFolder = StorageUtils.getLogsFolder(mApplication);
      if (!TextUtils.isEmpty(logsFolder))
        return new FileLoggerStrategy(mApplication,logsFolder + File.separator
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

  @NonNull
  public LogReceiver createLibnotifyLogger()
  {
    return new LibnotifyLogReceiver();
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
