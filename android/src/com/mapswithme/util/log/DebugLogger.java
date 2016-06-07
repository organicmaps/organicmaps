package com.mapswithme.util.log;

import com.mapswithme.maps.BuildConfig;

public class DebugLogger extends Logger
{
  private final Logger mLogger;

  public DebugLogger(String tag)
  {
    mLogger = (BuildConfig.DEBUG ? SimpleLogger.get(tag) : null);
  }

  @Override
  public void d(String message)
  {
    if (mLogger != null)
      mLogger.d(message);
  }

  @Override
  public void e(String message)
  {
    if (mLogger != null)
      mLogger.e(message);
  }

  @Override
  public void d(String message, Object... args)
  {
    if (mLogger != null)
      mLogger.d(message, args);
  }

  @Override
  public void e(String message, Object... args)
  {
    if (mLogger != null)
      mLogger.e(message, args);
  }
}
