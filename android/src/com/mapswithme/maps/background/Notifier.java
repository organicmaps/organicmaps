package com.mapswithme.maps.background;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.support.annotation.IntDef;
import android.support.annotation.Nullable;
import android.support.v4.app.NotificationCompat;

import com.mapswithme.maps.MwmActivity;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.util.StringUtils;
import com.mapswithme.util.statistics.Statistics;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

public final class Notifier
{
  private static final String EXTRA_CANCEL_NOTIFICATION = "extra_cancel_notification";
  private static final String EXTRA_NOTIFICATION_CLICKED = "extra_notification_clicked";
  private static final MwmApplication APP = MwmApplication.get();

  public static final int ID_NONE = 0;
  public static final int ID_DOWNLOAD_FAILED = 1;
  public static final int ID_IS_NOT_AUTHENTICATED = 2;

  @Retention(RetentionPolicy.SOURCE)
  @IntDef({ ID_NONE, ID_DOWNLOAD_FAILED, ID_IS_NOT_AUTHENTICATED })
  public @interface NotificationId
  {
  }

  private Notifier()
  {
  }

  public static void notifyDownloadFailed(@Nullable String id, @Nullable String name)
  {
    String title = APP.getString(R.string.app_name);
    String content = APP.getString(R.string.download_country_failed, name);

    PendingIntent pi = PendingIntent.getActivity(APP, 0, MwmActivity.createShowMapIntent(APP, id)
                                                                    .setFlags(Intent.FLAG_ACTIVITY_NEW_TASK),
                                                 PendingIntent.FLAG_UPDATE_CURRENT);

    placeNotification(title, content, pi, ID_DOWNLOAD_FAILED);
    Statistics.INSTANCE.trackEvent(Statistics.EventName.DOWNLOAD_COUNTRY_NOTIFICATION_SHOWN);
  }

  public static void notifyAuthentication()
  {
    Intent authIntent = MwmActivity.createAuthenticateIntent();
    authIntent.putExtra(EXTRA_CANCEL_NOTIFICATION, Notifier.ID_IS_NOT_AUTHENTICATED);
    authIntent.putExtra(EXTRA_NOTIFICATION_CLICKED,
                        Statistics.EventName.UGC_NOT_AUTH_NOTIFICATION_CLICKED);

    PendingIntent pi = PendingIntent.getActivity(APP, 0, authIntent,
                                                 PendingIntent.FLAG_UPDATE_CURRENT);

    NotificationCompat.Builder builder =
        getBuilder(APP.getString(R.string.notification_unsent_reviews_title),
                   APP.getString(R.string.notification_unsent_reviews_message), pi);

    builder.addAction(0, APP.getString(R.string.authorization_button_sign_in), pi);

    getNotificationManager().notify(ID_IS_NOT_AUTHENTICATED, builder.build());

    Statistics.INSTANCE.trackEvent(Statistics.EventName.UGC_NOT_AUTH_NOTIFICATION_SHOWN);
  }

  public static void cancelNotification(@NotificationId int id)
  {
    if (id == ID_NONE)
      return;

    getNotificationManager().cancel(id);
  }

  public static void processNotificationExtras(@Nullable Intent intent)
  {
    if (intent == null)
      return;

    if (intent.hasExtra(Notifier.EXTRA_CANCEL_NOTIFICATION))
    {
      @Notifier.NotificationId
      int notificationId = intent.getIntExtra(Notifier.EXTRA_CANCEL_NOTIFICATION, Notifier.ID_NONE);
      cancelNotification(notificationId);
    }

    if (intent.hasExtra(Notifier.EXTRA_NOTIFICATION_CLICKED))
    {
      String eventName = intent.getStringExtra(Notifier.EXTRA_NOTIFICATION_CLICKED);
      Statistics.INSTANCE.trackEvent(eventName);
    }
  }

  private static void placeNotification(String title, String content, PendingIntent pendingIntent,
                                        int notificationId)
  {
    final Notification notification = getBuilder(title, content, pendingIntent)
        .build();

    getNotificationManager().notify(notificationId, notification);
  }

  private static NotificationCompat.Builder getBuilder(String title, String content,
                                                       PendingIntent pendingIntent)
  {
    return new NotificationCompat.Builder(APP)
        .setAutoCancel(true)
        .setSmallIcon(R.drawable.ic_notification)
        .setColor(APP.getResources().getColor(R.color.base_accent))
        .setContentTitle(title)
        .setContentText(content)
        .setTicker(getTicker(title, content))
        .setContentIntent(pendingIntent);
  }

  private static CharSequence getTicker(String title, String content)
  {
    return APP.getString(StringUtils.isRtl() ? R.string.notification_ticker_rtl : R.string.notification_ticker_ltr, title, content);
  }

  private static NotificationManager getNotificationManager()
  {
    return (NotificationManager) APP.getSystemService(Context.NOTIFICATION_SERVICE);
  }
}
