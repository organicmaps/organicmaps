package com.mapswithme.maps.background;

import android.app.Application;
import android.support.annotation.NonNull;

public class StubNotificationChannelProvider implements NotificationChannelProvider
{
  private static final String DEFAULT_NOTIFICATION_CHANNEL = "default_notification_channel";

  @NonNull
  private final Application mApplication;

  @NonNull
  private final String mAuthChannel;

  @NonNull
  private final String mDownloadingChannel;


  StubNotificationChannelProvider(@NonNull Application context, @NonNull String authChannel,
                                  @NonNull String downloadingChannel)
  {
    mApplication = context;
    mAuthChannel = authChannel;
    mDownloadingChannel = downloadingChannel;
  }

  StubNotificationChannelProvider(@NonNull Application context)
  {
    this(context, DEFAULT_NOTIFICATION_CHANNEL, DEFAULT_NOTIFICATION_CHANNEL);
  }

  @Override
  @NonNull
  public String getUGCChannel()
  {
    return mAuthChannel;
  }

  @Override
  public void setUGCChannel()
  {
    /*Do nothing */
  }

  @NonNull
  @Override
  public String getDownloadingChannel()
  {
    return mDownloadingChannel;
  }

  @Override
  public void setDownloadingChannel()
  {
    /*Do nothing */
  }

  @NonNull
  protected Application getApplication()
  {
    return mApplication;
  }
}
