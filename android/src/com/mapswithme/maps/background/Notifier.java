package com.mapswithme.maps.background;

import android.app.Activity;
import android.app.AlarmManager;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.Uri;
import android.support.v4.app.NotificationCompat;

import com.mapswithme.maps.BuildConfig;
import com.mapswithme.maps.MWMActivity;
import com.mapswithme.maps.MWMApplication;
import com.mapswithme.maps.MapStorage.Index;
import com.mapswithme.maps.R;
import com.mapswithme.maps.guides.GuidesUtils;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.statistics.Statistics;

import java.util.Calendar;

public class Notifier
{
  private final static int ID_UPDATE_AVAIL = 0x1;
  private final static int ID_GUIDE_AVAIL = 0x2;
  private final static int ID_DOWNLOAD_STATUS = 0x3;
  private final static int ID_DOWNLOAD_NEW_COUNTRY = 0x4;
  private final static int ID_PRO_IS_FREE = 0x5;

  private static final String FREE_PROMO_SHOWN = "ProIsFreePromo";
  private static final String EXTRA_CONTENT = "ExtraContent";
  private static final String EXTRA_TITLE = "ExtraTitle";
  private static final String EXTRA_INTENT = "ExtraIntent";
  private static final String EXTRA_FORCE_PROMO_DIALOG = "ExtraForceDialog";

  public static final String ACTION_NAME = "com.mapswithme.MYACTION";
  private static IntentFilter mIntentFilter = new IntentFilter(ACTION_NAME);
  private static BroadcastReceiver mAlarmReceiver = new BroadcastReceiver()
  {
    @Override
    public void onReceive(Context context, Intent intent)
    {
      if (intent.getBooleanExtra(EXTRA_FORCE_PROMO_DIALOG, false))
        showFreeProNotification(new Intent(context, MWMActivity.class).putExtras(intent.getExtras()));
      else
        showFreeLiteNotification(new Intent(Intent.ACTION_VIEW, Uri.parse(BuildConfig.PRO_URL)).putExtras(intent.getExtras()));
    }
  };

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

    final PendingIntent pi = PendingIntent.getActivity(MWMApplication.get(), 0, MWMActivity.createUpdateMapsIntent(),
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

    placeDownloadNotification(title, content, idx);
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

  public static void notifyAboutFreePro(Activity activity)
  {
    if (BuildConfig.IS_PRO)
      freePromoInPro(activity);
    else
      freePromoInLite(activity);
  }

  private static void freePromoInLite(Activity activity)
  {
    final MWMApplication application = MWMApplication.get();
    final Calendar calendar = Calendar.getInstance();
    calendar.set(2014, Calendar.DECEMBER, 3, 19, 0);
    if (System.currentTimeMillis() > calendar.getTimeInMillis())
    {
      UiUtils.showDownloadProDialog(activity, application.getString(R.string.free_pro_version_notification_alert));
      cancelPromoNotifications();
    }
    else
      scheduleFreeLiteNotification(application.getString(R.string.free_pro_version_notification_lite), "", calendar);
  }

  private static void cancelPromoNotifications()
  {
    final MWMApplication application = MWMApplication.get();
    final AlarmManager alarmManager = (AlarmManager) application.getSystemService(Context.ALARM_SERVICE);
    final Intent it = new Intent(ACTION_NAME);
    final PendingIntent pi = PendingIntent.getBroadcast(application, 0, it, PendingIntent.FLAG_UPDATE_CURRENT);
    alarmManager.cancel(pi);
  }

  private static void scheduleFreeLiteNotification(String title, String content, Calendar calendar)
  {
    final MWMApplication application = MWMApplication.get();
    application.registerReceiver(mAlarmReceiver, mIntentFilter);
    final Intent it = new Intent(ACTION_NAME).
        putExtra(EXTRA_TITLE, title).
        putExtra(EXTRA_CONTENT, content);
    final PendingIntent pi = PendingIntent.getBroadcast(application, 0, it, PendingIntent.FLAG_UPDATE_CURRENT);

    final AlarmManager alarmManager = (AlarmManager) application.getSystemService(Context.ALARM_SERVICE);
    alarmManager.set(AlarmManager.RTC, calendar.getTimeInMillis(), pi);
  }

  public static void showFreeLiteNotification(Intent intent)
  {
    final String title = intent.getStringExtra(EXTRA_TITLE);
    final String content = intent.getStringExtra(EXTRA_CONTENT);
    final Notification notification = getBuilder()
        .setContentTitle(title)
        .setContentText(content)
        .setTicker(title + ": " + content)
        .setContentIntent(PendingIntent.getActivity(MWMApplication.get(), 0, intent, PendingIntent.FLAG_UPDATE_CURRENT))
        .build();

    getNotificationManager().notify(ID_PRO_IS_FREE, notification);
  }

  private static void freePromoInPro(Activity activity)
  {
    final MWMApplication application = MWMApplication.get();
    final Calendar calendar = Calendar.getInstance();
    calendar.set(2014, Calendar.DECEMBER, 3);
    if (Utils.isInstalledAfter(calendar) &&
        !application.nativeGetBoolean(FREE_PROMO_SHOWN, false))
    {
      if (application.getForegroundTime() > 10 * 60)
      {
        UiUtils.showPromoShareDialog(activity, application.getString(R.string.free_pro_version_share_message));
        application.nativeSetBoolean(FREE_PROMO_SHOWN, true);
        cancelPromoNotifications();
      }
      else
        scheduleFreeProNotification(application.getString(R.string.free_pro_version_notification_pro), "", calendar);
    }
  }

  private static void scheduleFreeProNotification(String title, String content, Calendar calendar)
  {
    final MWMApplication application = MWMApplication.get();
    application.registerReceiver(mAlarmReceiver, mIntentFilter);
    final Intent it = new Intent(ACTION_NAME).
        putExtra(EXTRA_TITLE, title).
        putExtra(EXTRA_CONTENT, content).
        putExtra(EXTRA_FORCE_PROMO_DIALOG, true);
    final PendingIntent pi = PendingIntent.getBroadcast(application, 0, it, PendingIntent.FLAG_UPDATE_CURRENT);

    final AlarmManager alarmManager = (AlarmManager) application.getSystemService(Context.ALARM_SERVICE);
    alarmManager.set(AlarmManager.RTC, calendar.getTimeInMillis(), pi);
  }

  public static void showFreeProNotification(Intent intent)
  {
    final MWMApplication application = MWMApplication.get();
    final PendingIntent pi = PendingIntent.getActivity(application, 0, new Intent(application, MWMActivity.class),
        PendingIntent.FLAG_UPDATE_CURRENT);
    final String title = intent.getStringExtra(EXTRA_TITLE);
    final String content = intent.getStringExtra(EXTRA_CONTENT);

    final Notification notification = getBuilder()
        .setContentTitle(title)
        .setContentText(content)
        .setTicker(title + ": " + content)
        .setContentIntent(pi)
        .build();

    getNotificationManager().notify(ID_PRO_IS_FREE, notification);
  }
}
