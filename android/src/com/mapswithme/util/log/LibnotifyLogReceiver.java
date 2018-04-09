package com.mapswithme.util.log;

import ru.mail.notify.core.utils.LogReceiver;

class LibnotifyLogReceiver implements LogReceiver
{
  private static final Logger LOGGER
      = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.THIRD_PARTY);
  private static final String TAG = "LIBNOTIFY_";
  @Override
  public void v(String tag, String msg)
  {
    LOGGER.v(TAG + tag, msg);
  }

  @Override
  public void v(String tag, String msg, Throwable throwable)
  {
    LOGGER.v(TAG + tag, msg, throwable);
  }

  @Override
  public void e(String tag, String msg)
  {
    LOGGER.e(TAG + tag, msg);
  }

  @Override
  public void e(String tag, String msg, Throwable throwable)
  {
    LOGGER.e(tag, msg, throwable);
  }

  @Override
  public void d(String tag, String msg)
  {
    LOGGER.d(TAG + tag, msg);
  }

  @Override
  public void d(String tag, String msg, Throwable throwable)
  {
    LOGGER.d(TAG + tag, msg, throwable);
  }
}
