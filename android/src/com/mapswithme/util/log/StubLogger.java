package com.mapswithme.util.log;

public class StubLogger extends Logger
{

  @Override
  public void d(String message)
  {
  }

  @Override
  public void e(String message)
  {
  }

  @Override
  public void d(String message, Object... args)
  {
  }

  @Override
  public void e(String message, Object... args)
  {
  }

  public static StubLogger get() { return new StubLogger(); }

}
