package app.organicmaps.background;

import android.app.Application;

import androidx.annotation.NonNull;

public class StubNotificationChannelProvider implements NotificationChannelProvider
{
  private static final String DEFAULT_NOTIFICATION_CHANNEL = "default_notification_channel";

  @NonNull
  private final Application mApplication;

  @NonNull
  private final String mDownloadingChannel;


  StubNotificationChannelProvider(@NonNull Application context, @NonNull String downloadingChannel)
  {
    mApplication = context;
    mDownloadingChannel = downloadingChannel;
  }

  StubNotificationChannelProvider(@NonNull Application context)
  {
    this(context, DEFAULT_NOTIFICATION_CHANNEL);
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
