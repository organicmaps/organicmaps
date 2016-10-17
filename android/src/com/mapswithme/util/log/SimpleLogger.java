package com.mapswithme.util.log;

import android.util.Log;

public class SimpleLogger extends Logger
{
  public static SimpleLogger get() { return new SimpleLogger(); }

  public static SimpleLogger get(String tag) { return new SimpleLogger(tag); }

  @Override
  public void d(String message)
  {
    d(message, (Object[]) null);
  }

  @Override
  public void e(String message)
  {
    e(message, (Object[]) null);
  }

  @Override
  public void d(String message, Object... args)
  {
    Log.d(tag, message + join(args));
  }

  @Override
  public void e(String message, Object... args)
  {
    Log.e(tag, message + join(args));
  }

  @Override
  public void e(Throwable throwable, String message, Object... args)
  {
    Log.e(tag, message + join(args), throwable);
  }

  private SimpleLogger() {}

  private SimpleLogger(String tag) { super(tag); }
}
