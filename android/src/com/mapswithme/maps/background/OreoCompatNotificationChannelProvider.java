package com.mapswithme.maps.background;

import android.annotation.TargetApi;
import android.app.Application;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.os.Build;
import android.support.annotation.NonNull;

import com.mapswithme.maps.R;

import java.util.Objects;

@TargetApi(Build.VERSION_CODES.O)
public class OreoCompatNotificationChannelProvider extends StubNotificationChannelProvider
{
  private static final String AUTH_NOTIFICATION_CHANNEL = "auth_notification_channel";
  private static final String DOWNLOADING_NOTIFICATION_CHANNEL = "downloading_notification_channel";

  OreoCompatNotificationChannelProvider(@NonNull Application app)
  {
    super(app, AUTH_NOTIFICATION_CHANNEL, DOWNLOADING_NOTIFICATION_CHANNEL);
  }

  @Override
  public void setUGCChannel()
  {
    String name = getApplication().getString(R.string.notification_channel_ugc);
    setChannelInternal(getUGCChannel(), name);
  }

  private void setChannelInternal(@NonNull String id, @NonNull String name)
  {
    NotificationManager notificationManager = getApplication().getSystemService(NotificationManager.class);
    NotificationChannel channel = Objects.requireNonNull(notificationManager)
                                         .getNotificationChannel(id);
    if (channel == null)
      channel = new NotificationChannel(id, name, NotificationManager.IMPORTANCE_DEFAULT);
    else
      channel.setName(name);
    notificationManager.createNotificationChannel(channel);
  }

  @Override
  public void setDownloadingChannel()
  {
    String name = getApplication().getString(R.string.notification_channel_downloader);
    setChannelInternal(getDownloadingChannel(), name);
  }
}
