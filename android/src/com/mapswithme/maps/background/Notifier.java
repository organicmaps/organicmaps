package com.mapswithme.maps.background;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.support.v4.app.NotificationCompat;

import com.mapswithme.country.DownloadActivity;
import com.mapswithme.maps.MWMActivity;
import com.mapswithme.maps.MWMApplication;
import com.mapswithme.maps.MapStorage.Index;
import com.mapswithme.maps.R;
import com.mapswithme.maps.guides.GuidesUtils;
import com.mapswithme.util.statistics.Statistics;

public class Notifier
{
  private final static int ID_UPDATE_AVAIL = 0x1;
  private final static int ID_GUIDE_AVAIL = 0x2;
  private final static int ID_DOWNLOAD_STATUS = 0x3;
  private final static int ID_DOWNLOAD_NEW_COUNTRY = 0x4;

  private Notifier() { }

  public static NotificationCompat.Builder getBuilder()
  {
    return new NotificationCompat.Builder(MWMApplication.get())
        .setAutoCancel(true)
        .setSmallIcon(R.drawable.ic_notification);
  }

  private static NotificationManager getNotificationManager()
  {
    return (NotificationManager) MWMApplication.get().getSystemService(Context.NOTIFICATION_SERVICE);
  }

  public static void placeUpdateAvailable(String forWhat)
  {
    final String title = MWMApplication.get().getString(R.string.advise_update_maps);

    // Intent to start DownloadUI
    final Intent i = new Intent(MWMApplication.get(), DownloadActivity.class).setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
    final PendingIntent pi = PendingIntent.getActivity(MWMApplication.get(), 0, i,
        PendingIntent.FLAG_UPDATE_CURRENT);

    final Notification notification = Notifier.getBuilder()
        .setContentTitle(title)
        .setContentText(forWhat)
        .setTicker(title + forWhat)
        .setContentIntent(pi)
        .build();

    getNotificationManager().cancel(ID_UPDATE_AVAIL);
    getNotificationManager().notify(ID_UPDATE_AVAIL, notification);
  }

  public static void placeDownloadCompleted(Index idx, String name)
  {
    final String title = MWMApplication.get().getString(R.string.app_name);
    final String content = MWMApplication.get().getString(R.string.download_country_success, name);

    Notifier.placeDownloadNotification(title, content, idx);
  }

  public static void placeDownloadFailed(Index idx, String name)
  {
    final String title = MWMApplication.get().getString(R.string.app_name);
    final String content = MWMApplication.get().getString(R.string.download_country_failed, name);

    placeDownloadNotification(title, content, idx);
  }

  private static void placeDownloadNotification(String title, String content, Index idx)
  {
    final PendingIntent pi = PendingIntent.getActivity(MWMApplication.get(), 0,
        MWMActivity.createShowMapIntent(MWMApplication.get(), idx, false).setFlags(Intent.FLAG_ACTIVITY_NEW_TASK),
        PendingIntent.FLAG_UPDATE_CURRENT);

    final Notification notification = getBuilder()
        .setContentTitle(title)
        .setContentText(content)
        .setTicker(title + ": " + content)
        .setContentIntent(pi)
        .build();

    getNotificationManager().notify(ID_DOWNLOAD_STATUS, notification);
  }

  public static void placeGuideAvailable(String packageName, String title, String content)
  {
    final PendingIntent pi = PendingIntent
        .getActivity(MWMApplication.get(), 0, GuidesUtils.getGoogleStoreIntentForPackage(packageName), 0);

    final Notification guideNotification = getBuilder()
        .setContentIntent(pi)
        .setContentTitle(title)
        .setContentText(content)
        .build();

    getNotificationManager().notify(ID_GUIDE_AVAIL, guideNotification);
  }

  public static void placeDownloadSuggest(String title, String content, Index countryIndex)
  {
    final PendingIntent pi = PendingIntent.getActivity(MWMApplication.get(), 0,
        MWMActivity.createShowMapIntent(MWMApplication.get(), countryIndex, true).setFlags(Intent.FLAG_ACTIVITY_NEW_TASK),
        PendingIntent.FLAG_UPDATE_CURRENT);

    final Notification notification = getBuilder()
        .setContentTitle(title)
        .setContentText(content)
        .setTicker(title + ": " + content)
        .setContentIntent(pi)
        .build();

    getNotificationManager().notify(ID_DOWNLOAD_NEW_COUNTRY, notification);

    Statistics.INSTANCE.trackDownloadCountryNotificationShown();
  }
}
