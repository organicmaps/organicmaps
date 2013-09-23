package com.mapswithme.util.log;

public abstract class Logger
{
  protected String tag = "MAPSWITHME";

  protected Logger() {}

  protected Logger(String tag)
  {
    this.tag = tag;
  }

  public abstract void d(String message);
  public abstract void e(String message);

  public abstract void d(String message, Object ... args);
  public abstract void e(String message, Object ... args);
}
