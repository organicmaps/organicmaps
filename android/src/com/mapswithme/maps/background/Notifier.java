package com.mapswithme.maps.background;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.support.v4.app.NotificationCompat;

import com.mapswithme.maps.MwmActivity;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.util.StringUtils;
import com.mapswithme.util.statistics.Statistics;

public final class Notifier
{
  private final static int ID_UPDATE_AVAILABLE = 1;
  private final static int ID_DOWNLOAD_FAILED = 2;
  private final static int ID_DOWNLOAD_NEW_COUNTRY = 3;

  private static final MwmApplication APP = MwmApplication.get();

  private Notifier() { }

  public static void notifyUpdateAvailable(String countriesName)
  {
    final String title = APP.getString(R.string.advise_update_maps);

    final PendingIntent pi = PendingIntent.getActivity(APP, 0, MwmActivity.createUpdateMapsIntent(),
        PendingIntent.FLAG_UPDATE_CURRENT);

    placeNotification(title, countriesName, pi, ID_UPDATE_AVAILABLE);
  }

  public static void notifyDownloadFailed(String id, String name)
  {
    String title = APP.getString(R.string.app_name);
    String content = APP.getString(R.string.download_country_failed, name);

    PendingIntent pi = PendingIntent.getActivity(APP, 0, MwmActivity.createShowMapIntent(APP, id, false)
                                                                    .setFlags(Intent.FLAG_ACTIVITY_NEW_TASK),
                                                 PendingIntent.FLAG_UPDATE_CURRENT);

    placeNotification(title, content, pi, ID_DOWNLOAD_FAILED);
    Statistics.INSTANCE.trackEvent(Statistics.EventName.DOWNLOAD_COUNTRY_NOTIFICATION_SHOWN);
  }

  public static void notifyDownloadSuggest(String title, String content, String countryId)
  {
    PendingIntent pi = PendingIntent.getActivity(APP, 0, MwmActivity.createShowMapIntent(APP, countryId, true)
                                                                    .setFlags(Intent.FLAG_ACTIVITY_NEW_TASK),
                                                 PendingIntent.FLAG_UPDATE_CURRENT);

    placeNotification(title, content, pi, ID_DOWNLOAD_NEW_COUNTRY);
    Statistics.INSTANCE.trackEvent(Statistics.EventName.DOWNLOAD_COUNTRY_NOTIFICATION_SHOWN);
  }

  public static void cancelDownloadFailed()
  {
    getNotificationManager().cancel(ID_DOWNLOAD_FAILED);
  }

  public static void cancelDownloadSuggest()
  {
    getNotificationManager().cancel(ID_DOWNLOAD_NEW_COUNTRY);
  }

  private static void placeNotification(String title, String content, PendingIntent pendingIntent, int notificationId)
  {
    final Notification notification = getBuilder(title, content, pendingIntent)
        .build();

    getNotificationManager().notify(notificationId, notification);
  }

  private static void placeBigNotification(String title, String content, PendingIntent pendingIntent, int notificationId)
  {
    final Notification notification = getBuilder(title, content, pendingIntent)
        .setStyle(new NotificationCompat.BigTextStyle().bigText(content))
        .build();

    getNotificationManager().notify(notificationId, notification);
  }

  private static NotificationCompat.Builder getBuilder(String title, String content, PendingIntent pendingIntent)
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
