package com.mapswithme.util.log;

import android.util.Log;
import net.jcip.annotations.Immutable;

@Immutable
class LogCatStrategy implements LoggerStrategy
{
  private final boolean mIsDebug;

  public LogCatStrategy(final boolean isDebug)
  {
    mIsDebug = isDebug;
  }

  @Override
  public void v(String tag, String msg)
  {
    if (mIsDebug)
      Log.v(tag, msg);
  }

  @Override
  public void v(String tag, String msg, Throwable tr)
  {
    if (mIsDebug)
      Log.v(tag, msg, tr);
  }

  @Override
  public void d(String tag, String msg)
  {
    if (mIsDebug)
      Log.d(tag, msg);
  }

  @Override
  public void d(String tag, String msg, Throwable tr)
  {
    if (mIsDebug)
      Log.d(tag, msg, tr);
  }

  @Override
  public void i(String tag, String msg)
  {
    Log.i(tag, msg);
  }

  @Override
  public void i(String tag, String msg, Throwable tr)
  {
    Log.i(tag, msg, tr);
  }

  @Override
  public void w(String tag, String msg)
  {
    Log.w(tag, msg);
  }

  @Override
  public void w(String tag, String msg, Throwable tr)
  {
    Log.w(tag, msg, tr);
  }

  @Override
  public void w(String tag, Throwable tr)
  {
    Log.w(tag, tr);
  }

  @Override
  public void e(String tag, String msg)
  {
    Log.e(tag, msg);
  }

  @Override
  public void e(String tag, String msg, Throwable tr)
  {
    Log.e(tag, msg, tr);
  }
}
