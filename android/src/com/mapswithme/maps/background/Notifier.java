package com.mapswithme.maps.background;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.support.v4.app.NotificationCompat;

import com.mapswithme.maps.DownloadUI;
import com.mapswithme.maps.R;

public class Notifier
{
  private final static int ID_UPDATE_AVAIL = 0x1;

  private NotificationManager mNotificationManager;
  private Context mContext;

  public Notifier(Context context)
  {
    mNotificationManager = (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE);
    mContext = context;
  }

  public void placeUpdateAvailable(String forWhat)
  {
    // TODO: add real resources
    final String title = "Map Update Available ";
    final String text = "Updated maps: " + forWhat;

    // Intent to start DownloadUI
    final Intent i = new Intent(mContext, DownloadUI.class);
    final PendingIntent pi = PendingIntent.getActivity(mContext, 0, i, Intent.FLAG_ACTIVITY_NEW_TASK);

    final Notification notification = getBuilder()
        .setContentTitle(title)
        .setContentText(text)
        .setTicker(title + text)
        .setContentIntent(pi)
        .build();

    mNotificationManager.notify(ID_UPDATE_AVAIL, notification);
  }


  public NotificationCompat.Builder getBuilder()
  {
    // TODO: add default initialization
    return new NotificationCompat.Builder(mContext)
      .setAutoCancel(true)
      .setSmallIcon(R.drawable.ic_launcher);
  }

}
