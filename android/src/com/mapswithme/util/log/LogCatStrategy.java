package com.mapswithme.util.log;

import android.util.Log;

import com.mapswithme.maps.BuildConfig;
import net.jcip.annotations.Immutable;

@Immutable
class LogCatStrategy implements LoggerStrategy
{
  @Override
  public void v(String tag, String msg)
  {
    if (canLog())
      Log.v(tag, msg);
  }

  @Override
  public void v(String tag, String msg, Throwable tr)
  {
    if (canLog())
      Log.v(tag, msg, tr);
  }

  @Override
  public void d(String tag, String msg)
  {
    if (canLog())
      Log.d(tag, msg);
  }

  @Override
  public void d(String tag, String msg, Throwable tr)
  {
    if (canLog())
      Log.d(tag, msg, tr);
  }

  @Override
  public void i(String tag, String msg)
  {
    if (canLog())
      Log.i(tag, msg);
  }

  @Override
  public void i(String tag, String msg, Throwable tr)
  {
    if (canLog())
      Log.i(tag, msg, tr);
  }

  @Override
  public void w(String tag, String msg)
  {
    if (canLog())
      Log.w(tag, msg);
  }

  @Override
  public void w(String tag, String msg, Throwable tr)
  {
    if (canLog())
      Log.w(tag, msg, tr);
  }

  @Override
  public void w(String tag, Throwable tr)
  {
    if (canLog())
      Log.w(tag, tr);
  }

  @Override
  public void e(String tag, String msg)
  {
    if (canLog())
      Log.e(tag, msg);
  }

  @Override
  public void e(String tag, String msg, Throwable tr)
  {
    if (canLog())
      Log.e(tag, msg, tr);
  }

  private static boolean canLog()
  {
    return BuildConfig.DEBUG;
  }
}
