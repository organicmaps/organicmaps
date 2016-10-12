package com.mapswithme.util.log;

import android.text.TextUtils;

public abstract class Logger
{
  protected String tag = "MAPSWITHME";

  protected Logger() {}

  protected Logger(String tag)
  {
    this.tag = tag;
  }

  static protected String join(Object... args)
  {
    return (args != null ? TextUtils.join(", ", args) : "");
  }

  public abstract void d(String message);

  public abstract void e(String message);

  public abstract void d(String message, Object... args);

  public abstract void e(String message, Object... args);

  public abstract void e(Throwable throwable, String message, Object... args);
}
