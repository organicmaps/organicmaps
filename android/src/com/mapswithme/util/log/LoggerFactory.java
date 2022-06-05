package com.mapswithme.util.log;

import android.app.Application;
import android.content.Context;
import android.content.SharedPreferences;
import android.location.LocationManager;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.os.Build;
import android.util.Log;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.mapswithme.maps.BuildConfig;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.util.Utils;
import net.jcip.annotations.GuardedBy;
import net.jcip.annotations.ThreadSafe;

import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.util.EnumMap;
import java.util.Locale;
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
    ensureLogsFolder();

    final SharedPreferences prefs = MwmApplication.prefs(mApplication);
    // File logging is enabled by default for beta builds.
    isFileLoggingEnabled = prefs.getBoolean(mApplication.getString(R.string.pref_enable_logging),
                                            BuildConfig.BUILD_TYPE.equals("beta"));
    getLogger(Type.MISC).i(TAG, "Logging config: isFileLoggingEnabled: " + isFileLoggingEnabled +
                                "; logs folder: " + mLogsFolder);

    // Set native logging level, save into shared preferences, update already created loggers if any.
    switchFileLoggingEnabled(isFileLoggingEnabled);
  }

  /**
   * Ensures logs folder exists.
   * Tries to create it and/or re-get a path from the system, falling back to the internal storage.
   * Switches off file logging if nothing had helped.
   *
   * Its important to have only system logging here to avoid infinite loop
   * (file loggers call ensureLogsFolder() in preparation to write).
   *
   * @return logs folder path, null if it can't be created
   *
   * NOTE: initFileLogging() must be called before.
   */
  @Nullable
  public synchronized String ensureLogsFolder()
  {
    assert mApplication != null : "mApplication must be initialized first by calling initFileLogging()";

    if (mLogsFolder != null && createWritableDir(mLogsFolder))
      return mLogsFolder;

    mLogsFolder = null;
    mLogsFolder = createLogsFolder(mApplication.getExternalFilesDir(null));
    if (mLogsFolder == null)
      mLogsFolder = createLogsFolder(mApplication.getFilesDir());

    if (mLogsFolder == null)
    {
      Log.e(TAG, "Can't create any logs folder");
      if (isFileLoggingEnabled)
        switchFileLoggingEnabled(false);
    }

    return mLogsFolder;
  }

  // Only system logging allowed, see ensureLogsFolder().
  private synchronized boolean createWritableDir(@NonNull final String path)
  {
    final File dir = new File(path);
    if (!dir.exists() && !dir.mkdirs())
    {
      Log.e(TAG, "Can't create a logs folder " + path);
      return false;
    }
    if (!dir.canWrite())
    {
      Log.e(TAG, "Can't write to a logs folder " + path);
      return false;
    }
    return true;
  }

  // Only system logging allowed, see ensureLogsFolder().
  @Nullable
  private synchronized String createLogsFolder(@Nullable final File dir)
  {
    if (dir != null)
    {
      final String path = dir.getPath() + File.separator + "logs";
      if (createWritableDir(path))
        return path;
    }
    return null;
  }

  // Only system logging allowed, see ensureLogsFolder().
  private synchronized void switchFileLoggingEnabled(boolean enabled)
  {
    enabled = enabled && mLogsFolder != null;
    Log.i(TAG, "Switch isFileLoggingEnabled to " + enabled);
    isFileLoggingEnabled = enabled;
    nativeToggleCoreDebugLogs(enabled || BuildConfig.DEBUG);
    MwmApplication.prefs(mApplication)
                  .edit()
                  .putBoolean(mApplication.getString(R.string.pref_enable_logging), enabled)
                  .apply();
    updateLoggers();
    Log.i(TAG, "File logging " + (enabled ? "started to " + mLogsFolder : "stopped"));
  }

  /**
   * Returns false if file logging can't be enabled.
   *
   * NOTE: initFileLogging() must be called before.
   */
  public boolean setFileLoggingEnabled(boolean enabled)
  {
    assert mApplication != null : "mApplication must be initialized first by calling initFileLogging()";

    if (isFileLoggingEnabled != enabled)
    {
      if (enabled && ensureLogsFolder() == null)
      {
        Log.e(TAG, "Can't enable file logging: there is no logs folder.");
        return false;
      }
      else
        switchFileLoggingEnabled(enabled);
    }

    return true;
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
    for (Type type : mLoggers.keySet())
      mLoggers.get(type).setStrategy(createLoggerStrategy(type));
  }

  /**
   * NOTE: initFileLogging() must be called before.
   */
  public synchronized void zipLogs(@NonNull OnZipCompletedListener listener)
  {
    assert mApplication != null : "mApplication must be initialized first by calling initFileLogging()";

    if (ensureLogsFolder() == null)
    {
      Log.e(TAG, "Can't send logs: there is no logs folder.");
      listener.onCompleted(false, null);
      return;
    }

    final Runnable task = new ZipLogsTask(mLogsFolder, mLogsFolder + ".zip", listener);
    getFileLoggerExecutor().execute(task);
  }

  @NonNull
  private BaseLogger createLogger(@NonNull Type type)
  {
    return new BaseLogger(createLoggerStrategy(type));
  }

  @NonNull
  private LoggerStrategy createLoggerStrategy(@NonNull Type type)
  {
    if (isFileLoggingEnabled)
      return new FileLoggerStrategy(type.name().toLowerCase() + ".log", getFileLoggerExecutor());

    return new LogCatStrategy(BuildConfig.DEBUG);
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
    final Logger logger = INSTANCE.getLogger(Type.CORE);
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

  /**
   * NOTE: initFileLogging() must be called before.
   */
  public void writeSystemInformation(@NonNull final FileWriter fw) throws IOException
  {
    assert mApplication != null : "mApplication must be initialized first by calling initFileLogging()";

    fw.write("Android version: " + Build.VERSION.SDK_INT);
    fw.write("\nDevice: " + Utils.getFullDeviceModel());
    fw.write("\nApp version: " + BuildConfig.APPLICATION_ID + " " + BuildConfig.VERSION_NAME);
    fw.write("\nLocale: " + Locale.getDefault());
    fw.write("\nNetworks: ");
    final ConnectivityManager manager = (ConnectivityManager) mApplication.getSystemService(Context.CONNECTIVITY_SERVICE);
    if (manager != null)
      // TODO: getAllNetworkInfo() is deprecated, for alternatives check
      // https://stackoverflow.com/questions/32547006/connectivitymanager-getnetworkinfoint-deprecated
      for (NetworkInfo info : manager.getAllNetworkInfo())
        fw.write(info.toString());
    fw.write("\nLocation providers: ");
    final LocationManager locMngr = (android.location.LocationManager) mApplication.getSystemService(Context.LOCATION_SERVICE);
    if (locMngr != null)
      for (String provider: locMngr.getProviders(true))
        fw.write(provider + " ");
    fw.write("\n\n");
  }

  private static native void nativeToggleCoreDebugLogs(boolean enabled);
}
