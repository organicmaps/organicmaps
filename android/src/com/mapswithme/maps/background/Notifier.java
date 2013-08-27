package com.mapswithme.maps.background;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.support.v4.app.NotificationCompat;

import com.mapswithme.maps.DownloadUI;
import com.mapswithme.maps.MWMActivity;
import com.mapswithme.maps.MapStorage.Index;
import com.mapswithme.maps.R;
import com.mapswithme.maps.guides.GuidesUtils;

public class Notifier
{
  private final static int ID_UPDATE_AVAIL = 0x1;
  private final static int ID_GUIDE_AVAIL  = 0x2;
  private final static int ID_DOWNLOAD_STATUS  = 0x3;

  private final NotificationManager mNotificationManager;
  private final Context mContext;

  public Notifier(Context context)
  {
    mNotificationManager = (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE);
    mContext = context;
  }

  public void placeUpdateAvailable(String forWhat)
  {
    final String title = mContext.getString(R.string.advise_update_maps);

    // Intent to start DownloadUI
    final Intent i = new Intent(mContext, DownloadUI.class);
    final PendingIntent pi = PendingIntent.getActivity(mContext, 0, i, Intent.FLAG_ACTIVITY_NEW_TASK);

    final Notification notification = getBuilder()
        .setContentTitle(title)
        .setContentText(forWhat)
        .setTicker(title + forWhat)
        .setContentIntent(pi)
        .build();

    mNotificationManager.notify(ID_UPDATE_AVAIL, notification);
  }

  public void placeDownloadCompleted(Index idx, String name)
  {
    final String title = mContext.getString(R.string.app_name);
    final String content = mContext.getString(R.string.download_country_success, name);

    placeDownloadNoti(title, content, idx);
  }

  public void placeDownloadFailed(Index idx, String name)
  {
    final String title = mContext.getString(R.string.app_name);
    final String content = mContext.getString(R.string.download_country_failed, name);

    placeDownloadNoti(title, content, idx);
  }

  private void placeDownloadNoti(String title, String content, Index idx)
  {
    final PendingIntent pi = PendingIntent
        .getActivity(mContext, 0, MWMActivity.createShowMapIntent(mContext, idx), Intent.FLAG_ACTIVITY_NEW_TASK);

    final Notification notification = getBuilder()
        .setContentTitle(title)
        .setContentText(content)
        .setTicker(title + content)
        .setContentIntent(pi)
        .build();

    mNotificationManager.notify(ID_DOWNLOAD_STATUS, notification);
  }

  public NotificationCompat.Builder getBuilder()
  {
    return new NotificationCompat.Builder(mContext)
      .setAutoCancel(true)
      .setSmallIcon(R.drawable.ic_notification);
  }

  public void placeGuideAvailable(String packageName, String title, String content)
  {
    final PendingIntent pi = PendingIntent
        .getActivity(mContext, 0, GuidesUtils.getGoogleStoreIntentForPackage(packageName), 0);

    final Notification guideNoti = getBuilder()
      .setContentIntent(pi)
      .setContentTitle(title)
      .setContentText(content)
      .build();

    mNotificationManager.notify(ID_GUIDE_AVAIL, guideNoti);
  }

}
