package com.mapswithme.util.log;

import androidx.annotation.NonNull;

import net.jcip.annotations.GuardedBy;
import net.jcip.annotations.ThreadSafe;

@ThreadSafe
class BaseLogger implements Logger
{
  @NonNull
  @GuardedBy("this")
  private LoggerStrategy mStrategy;

  BaseLogger(@NonNull LoggerStrategy strategy)
  {
    mStrategy = strategy;
  }

  synchronized void setStrategy(@NonNull LoggerStrategy strategy)
  {
    mStrategy = strategy;
  }

  @Override
  public synchronized void v(String tag, String msg)
  {
    mStrategy.v(tag, msg);
  }

  @Override
  public synchronized void v(String tag, String msg, Throwable tr)
  {
    mStrategy.v(tag, msg, tr);
  }

  @Override
  public synchronized void d(String tag, String msg)
  {
    mStrategy.d(tag, msg);
  }

  @Override
  public synchronized void d(String tag, String msg, Throwable tr)
  {
    mStrategy.d(tag, msg, tr);
  }

  @Override
  public synchronized void i(String tag, String msg)
  {
    mStrategy.i(tag, msg);
  }

  @Override
  public synchronized void i(String tag, String msg, Throwable tr)
  {
    mStrategy.i(tag, msg, tr);
  }

  @Override
  public synchronized void w(String tag, String msg)
  {
    mStrategy.w(tag, msg);
  }

  @Override
  public synchronized void w(String tag, String msg, Throwable tr)
  {
    mStrategy.w(tag, msg, tr);
  }

  @Override
  public synchronized void w(String tag, Throwable tr)
  {
    mStrategy.w(tag, tr);
  }

  @Override
  public synchronized void e(String tag, String msg)
  {
    mStrategy.e(tag, msg);
  }

  @Override
  public synchronized void e(String tag, String msg, Throwable tr)
  {
    mStrategy.e(tag, msg, tr);
  }
}
