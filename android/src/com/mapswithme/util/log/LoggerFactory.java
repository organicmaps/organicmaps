package com.mapswithme.util.log;

import android.app.Application;
import android.content.SharedPreferences;
import android.text.TextUtils;
import android.util.Log;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.mapswithme.maps.BuildConfig;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.util.StorageUtils;
import net.jcip.annotations.GuardedBy;
import net.jcip.annotations.ThreadSafe;

import java.io.File;
import java.util.EnumMap;
import java.util.Objects;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/**
 * By default uses Android's system logger.
 * After an initFileLogging() call can use a custom file logging implementation.
 */
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
    // Called from the logger thread.
    public void onCompleted(final boolean success, @Nullable final String zipPath);
  }

  public final static LoggerFactory INSTANCE = new LoggerFactory();
  public boolean isFileLoggingEnabled = false;

  @NonNull
  @GuardedBy("this")
  private final EnumMap<Type, BaseLogger> mLoggers = new EnumMap<>(Type.class);
  private final static String TAG = LoggerFactory.class.getSimpleName();
  private final static String CORE_TAG = "OMcore";
  @Nullable
  @GuardedBy("this")
  private ExecutorService mFileLoggerExecutor;
  @Nullable
  private Application mApplication;
  private String mLogsFolder;

  private LoggerFactory()
  {
    Log.i(LoggerFactory.class.getSimpleName(), "Logging started");
  }

  public void initFileLogging(@NonNull Application application)
  {
    getLogger(Type.MISC).i(TAG, "Init file logging");
    mApplication = application;
    mLogsFolder = StorageUtils.getLogsFolder(mApplication);

    final SharedPreferences prefs = MwmApplication.prefs(mApplication);
    // File logging is enabled by default for beta builds.
    isFileLoggingEnabled = prefs.getBoolean(mApplication.getString(R.string.pref_enable_logging),
                                            BuildConfig.BUILD_TYPE.equals("beta"));
    getLogger(Type.MISC).i(TAG, "Logging config: isFileLoggingEnabled: " + isFileLoggingEnabled +
                                "; logs folder: " + mLogsFolder);

    // Set native logging level, save into shared preferences, update already created loggers if any.
    switchFileLoggingEnabled(isFileLoggingEnabled);
  }

  private void switchFileLoggingEnabled(boolean enabled)
  {
    getLogger(Type.MISC).i(TAG, "Switch isFileLoggingEnabled to " + enabled);
    isFileLoggingEnabled = enabled;
    nativeToggleCoreDebugLogs(enabled || BuildConfig.DEBUG);
    MwmApplication.prefs(mApplication)
                  .edit()
                  .putBoolean(mApplication.getString(R.string.pref_enable_logging), enabled)
                  .apply();
    updateLoggers();
    getLogger(Type.MISC).i(TAG, "File logging " + (enabled ? "started to " + mLogsFolder : "stopped"));
  }

  /**
   * Throws a NullPointerException if initFileLogging() was not called before.
   */
  public void setFileLoggingEnabled(boolean enabled)
  {
    Objects.requireNonNull(mApplication);
    if (isFileLoggingEnabled != enabled)
      switchFileLoggingEnabled(enabled);
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

  /**
   * Does nothing if initFileLogging() was not called before.
   */
  public synchronized void zipLogs(@Nullable OnZipCompletedListener listener)
  {
    if (mApplication == null)
      return;

    if (TextUtils.isEmpty(mLogsFolder))
    {
      if (listener != null)
        listener.onCompleted(false, null);
      return;
    }

    Runnable task = new ZipLogsTask(mApplication, mLogsFolder, mLogsFolder + ".zip", listener);
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
    if (isFileLoggingEnabled && mApplication != null)
    {
      if (!TextUtils.isEmpty(mLogsFolder))
      {
        return new FileLoggerStrategy(mApplication, mLogsFolder + File.separator + type.name()
                                                                                       .toLowerCase() + ".log", getFileLoggerExecutor());
      }
    }
    return new LogCatStrategy(isFileLoggingEnabled || BuildConfig.DEBUG);
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
