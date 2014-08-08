package com.mapswithme.maps.background;

import android.app.AlarmManager;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.os.Build;
import android.support.v4.app.NotificationCompat;

import com.mapswithme.country.DownloadActivity;
import com.mapswithme.maps.MWMActivity;
import com.mapswithme.maps.MWMApplication;
import com.mapswithme.maps.MapStorage.Index;
import com.mapswithme.maps.R;
import com.mapswithme.maps.guides.GuidesUtils;
import com.mapswithme.util.statistics.Statistics;

import java.util.Calendar;

public class Notifier
{
  private final static int ID_UPDATE_AVAIL = 0x1;
  private final static int ID_GUIDE_AVAIL = 0x2;
  private final static int ID_DOWNLOAD_STATUS = 0x3;
  private final static int ID_DOWNLOAD_NEW_COUNTRY = 0x4;
  private final static int ID_MWM_PRO_PROMOACTION = 0x5;

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
    final Intent i = new Intent(MWMApplication.get(), DownloadActivity.class);
    final PendingIntent pi = PendingIntent.getActivity(MWMApplication.get(), 0, i, Intent.FLAG_ACTIVITY_NEW_TASK);

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
    final PendingIntent pi = PendingIntent
        .getActivity(MWMApplication.get(), 0, MWMActivity.createShowMapIntent(MWMApplication.get(), idx, false), Intent.FLAG_ACTIVITY_NEW_TASK);

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
    final PendingIntent pi = PendingIntent
        .getActivity(MWMApplication.get(), 0, MWMActivity.createShowMapIntent(MWMApplication.get(), countryIndex, true), Intent.FLAG_ACTIVITY_NEW_TASK);

    final Notification notification = getBuilder()
        .setContentTitle(title)
        .setContentText(content)
        .setTicker(title + ": " + content)
        .setContentIntent(pi)
        .build();

    getNotificationManager().notify(ID_DOWNLOAD_NEW_COUNTRY, notification);

    Statistics.INSTANCE.trackDownloadCountryNotificationShown();
  }

  public static void schedulePromoNotification()
  {
    final int promoYear = 2014;
    final int promoDate = 17;
    final int promoHour = 12;
    final Calendar calendar = Calendar.getInstance();
    calendar.set(Calendar.YEAR, promoYear);
    calendar.set(Calendar.MONTH, Calendar.AUGUST);
    calendar.set(Calendar.DAY_OF_MONTH, promoDate);
    calendar.set(Calendar.HOUR_OF_DAY, promoHour);
    calendar.set(Calendar.MINUTE, 0);
    calendar.set(Calendar.SECOND, 0);

    if (System.currentTimeMillis() < calendar.getTimeInMillis())
    {
      final Intent intent = new Intent(MWMApplication.get(), WorkerService.class).
          setAction(WorkerService.ACTION_PROMO_NOTIFICATION_SHOW);
      final PendingIntent pendingIntent = PendingIntent.getService(MWMApplication.get(), 0, intent, 0);

      final AlarmManager alarm = (AlarmManager) MWMApplication.get().getSystemService(Context.ALARM_SERVICE);
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT)
        alarm.setExact(AlarmManager.RTC_WAKEUP, calendar.getTimeInMillis(), pendingIntent);
      else
        alarm.set(AlarmManager.RTC_WAKEUP, calendar.getTimeInMillis(), pendingIntent);
    }
  }

  public static void placePromoNotification()
  {
    final Intent intent = new Intent(MWMApplication.get(), WorkerService.class).
        setAction(WorkerService.ACTION_PROMO_NOTIFICATION_CLICK);
    final PendingIntent pendingIntent = PendingIntent.getService(MWMApplication.get(), 0, intent, 0);

    // TODO add correct string from resources after translations
    final Notification notification = getBuilder()
        .setContentTitle("promo action")
        .setContentText("download mwmpro for 1 dollar!")
        .setTicker("promo action" + ": " + "download mwmpro for 1 dollar")
        .setContentIntent(pendingIntent)
        .build();

    getNotificationManager().notify(ID_MWM_PRO_PROMOACTION, notification);
  }
}
