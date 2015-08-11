package com.mapswithme.maps.background;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.support.v4.app.NotificationCompat;

import com.mapswithme.maps.MapStorage.Index;
import com.mapswithme.maps.MwmActivity;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.util.statistics.Statistics;

public final class Notifier
{
  private final static int ID_UPDATE_AVAIL = 0x1;
  private final static int ID_DOWNLOAD_FAILED = 0x3;
  private final static int ID_DOWNLOAD_NEW_COUNTRY = 0x4;

  private static final MwmApplication APP = MwmApplication.get();

  private Notifier() { }

  public static NotificationCompat.Builder getBuilder()
  {
    return new NotificationCompat.Builder(APP)
        .setAutoCancel(true)
        .setSmallIcon(R.drawable.ic_notification);
  }

  private static NotificationManager getNotificationManager()
  {
    return (NotificationManager) APP.getSystemService(Context.NOTIFICATION_SERVICE);
  }

  public static void notifyUpdateAvailable(String countryName)
  {
    final String title = APP.getString(R.string.advise_update_maps);

    final PendingIntent pi = PendingIntent.getActivity(APP, 0, MwmActivity.createUpdateMapsIntent(),
        PendingIntent.FLAG_UPDATE_CURRENT);

    placeNotification(title, countryName, pi, ID_UPDATE_AVAIL);
  }

  public static void notifyDownloadFailed(Index idx, String countryName)
  {
    final String title = APP.getString(R.string.app_name);
    final String content = APP.getString(R.string.download_country_failed, countryName);

    final PendingIntent pi = PendingIntent.getActivity(APP, 0,
        MwmActivity.createShowMapIntent(APP, idx, false).setFlags(Intent.FLAG_ACTIVITY_NEW_TASK),
        PendingIntent.FLAG_UPDATE_CURRENT);

    placeNotification(title, content, pi, ID_DOWNLOAD_FAILED);
  }

  public static void notifyDownloadSuggest(String title, String content, Index countryIndex)
  {
    final PendingIntent pi = PendingIntent.getActivity(APP, 0,
        MwmActivity.createShowMapIntent(APP, countryIndex, true).setFlags(Intent.FLAG_ACTIVITY_NEW_TASK),
        PendingIntent.FLAG_UPDATE_CURRENT);

    placeNotification(title, content, pi, ID_DOWNLOAD_NEW_COUNTRY);

    Statistics.INSTANCE.trackDownloadCountryNotificationShown();
  }

  private static void placeNotification(String title, String content, PendingIntent pendingIntent, int notificationId)
  {
    final Notification notification = getBuilder()
        .setContentTitle(title)
        .setContentText(content)
        .setTicker(title + ": " + content)
        .setContentIntent(pendingIntent)
        .build();

    getNotificationManager().notify(notificationId, notification);
  }

  public static void cancelDownloadSuggest()
  {
    getNotificationManager().cancel(ID_DOWNLOAD_NEW_COUNTRY);
  }
}
