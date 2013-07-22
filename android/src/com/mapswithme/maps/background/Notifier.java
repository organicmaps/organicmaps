package com.mapswithme.maps.background;

import java.util.List;

import com.mapswithme.maps.R;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.SimpleLogger;

import android.app.Notification;
import android.app.NotificationManager;
import android.content.Context;
import android.support.v4.app.NotificationCompat;
import android.text.TextUtils;

public class Notifier
{
  private Logger l = SimpleLogger.get("MWMNotification");

  private final static int ID_DEF = 0x0;
  private final static int ID_UPDATE_AVAIL = 0x1;

  private NotificationManager mNotificationManager;
  private Context mContext;

  public Notifier(Context context)
  {
    mNotificationManager = (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE);
    mContext = context;
  }

  public void placeNotification(String text)
  {
    l.d("Noti", text);

    final Notification notification = getBuilder()
        .setContentText(text)
        .setTicker(text)
        .setContentTitle(text)
        .build();

    mNotificationManager.notify(ID_DEF, notification);
  }

  public void placeUpdateAvailable(List<CharSequence> forItems)
  {
    // TODO add real resources
    final String forItemsStr = TextUtils.join(",", forItems);
    final String title = "Map Update Available ";
    final String text = "New data available for " + forItemsStr;
    final Notification notification = getBuilder()
        .setContentTitle(title)
        .setSubText(text)
        .setTicker(title + text)
        .build();

    mNotificationManager.notify(ID_UPDATE_AVAIL, notification);
  }

  public NotificationCompat.Builder getBuilder()
  {
    // TODO add default initialization
    return new NotificationCompat.Builder(mContext)
      .setAutoCancel(true)
      .setSmallIcon(R.drawable.ic_launcher);
  }

}
