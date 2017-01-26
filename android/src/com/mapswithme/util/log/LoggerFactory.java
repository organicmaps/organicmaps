package com.mapswithme.util.log;

import android.support.annotation.NonNull;
import android.text.TextUtils;

import com.mapswithme.util.Config;
import com.mapswithme.util.StorageUtils;
import net.jcip.annotations.GuardedBy;
import net.jcip.annotations.ThreadSafe;

import java.io.File;
import java.util.EnumMap;

@ThreadSafe
public class LoggerFactory
{
  public enum Type
  {
    MISC, LOCATION, TRAFFIC, GPS_TRACKING, TRACK_RECORDER, ROUTING, NETWORK;
  }

  public final static LoggerFactory INSTANCE = new LoggerFactory();
  @NonNull
  @GuardedBy("this")
  private final EnumMap<Type, BaseLogger> mLoggers = new EnumMap<>(Type.class);

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
      String externalDir = StorageUtils.getExternalFilesDir();
      if (!TextUtils.isEmpty(externalDir))
      {
        File folder = new File(externalDir + "/logs");
        boolean success = true;
        if (!folder.exists())
          success = folder.mkdir();
        if (success)
          return new FileLoggerStrategy(folder + File.separator + type.name().toLowerCase() + ".log");
      }
    }

    return new LogCatStrategy();
  }

  public synchronized void updateLoggers()
  {
    for (Type type: mLoggers.keySet())
    {
      BaseLogger logger = mLoggers.get(type);
      logger.setStrategy(createLoggerStrategy(type));
    }
  }
}
