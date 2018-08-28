package com.mapswithme.maps.background;

import android.annotation.TargetApi;
import android.app.Application;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.os.Build;
import android.support.annotation.NonNull;

import com.mapswithme.maps.R;
import com.mapswithme.util.Utils;

public interface NotificationChannelProvider
{
  @NonNull
  String getAuthChannel();

  void setAuthChannel();

  @NonNull
  String getDownloadingChannel();

  void setDownloadingChannel();

  class DefaultNotificationChannelProvider implements NotificationChannelProvider
  {
    private static final String DEFAULT_NOTIFICATION_CHANNEL = "default_notification_channel";

    @NonNull
    private final Application mApplication;

    @NonNull
    private final String mAuthChannel;

    @NonNull
    private final String mDownloadingChannel;


    DefaultNotificationChannelProvider(@NonNull Application context, @NonNull String authChannel,
                                       @NonNull String downloadingChannel)
    {
      mApplication = context;
      mAuthChannel = authChannel;
      mDownloadingChannel = downloadingChannel;
    }

    DefaultNotificationChannelProvider(@NonNull Application context)
    {
      this(context, DEFAULT_NOTIFICATION_CHANNEL, DEFAULT_NOTIFICATION_CHANNEL);
    }

    @Override
    @NonNull
    public String getAuthChannel()
    {
      return mAuthChannel;
    }

    @Override
    public void setAuthChannel()
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

  @NonNull
  static NotificationChannelProvider from(@NonNull Application app)
  {
    return Utils.isOreoOrLater() ? new OreoCompatProvider(app)
                                 : new DefaultNotificationChannelProvider(app);
  }

  @TargetApi(Build.VERSION_CODES.O)
  class OreoCompatProvider extends DefaultNotificationChannelProvider
  {
    private static final String AUTH_NOTIFICATION_CHANNEL = "auth_notification_channel";
    private static final String DOWNLOADING_NOTIFICATION_CHANNEL = "downloading_notification_channel";

    OreoCompatProvider(@NonNull Application app)
    {
      super(app, AUTH_NOTIFICATION_CHANNEL, DOWNLOADING_NOTIFICATION_CHANNEL);
    }

    @Override
    public void setAuthChannel()
    {
      String name = getApplication().getString(R.string.notification_unsent_reviews_title);
      setChannelInternal(getAuthChannel(), name);
    }

    private void setChannelInternal(@NonNull String id, @NonNull String name)
    {
      NotificationManager notificationManager = getApplication().getSystemService(NotificationManager.class);
      NotificationChannel channel = notificationManager.getNotificationChannel(id);
      if (channel == null)
        channel = new NotificationChannel(id, name, NotificationManager.IMPORTANCE_DEFAULT);
      else
        channel.setName(name);
      notificationManager.createNotificationChannel(channel);
    }


    @Override
    public void setDownloadingChannel()
    {
      String name = "NEED STRING ID FOR CHANNEL";
      setChannelInternal(getDownloadingChannel(), name);
    }
  }
}
