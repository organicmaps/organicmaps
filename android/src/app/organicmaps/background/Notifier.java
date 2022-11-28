package app.organicmaps.background;

import android.app.Application;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.os.Build;

import androidx.annotation.IntDef;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.app.NotificationCompat;

import app.organicmaps.MwmActivity;
import app.organicmaps.R;
import app.organicmaps.util.StringUtils;
import app.organicmaps.util.UiUtils;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

public final class Notifier
{
  private static final String EXTRA_CANCEL_NOTIFICATION = "extra_cancel_notification";
  private static final String EXTRA_NOTIFICATION_CLICKED = "extra_notification_clicked";

  public static final int ID_NONE = 0;
  public static final int ID_DOWNLOAD_FAILED = 1;
  public static final int ID_IS_NOT_AUTHENTICATED = 2;
  public static final int ID_LEAVE_REVIEW = 3;
  @NonNull
  private final Application mContext;

  @Retention(RetentionPolicy.SOURCE)
  @IntDef({ ID_NONE, ID_DOWNLOAD_FAILED, ID_IS_NOT_AUTHENTICATED, ID_LEAVE_REVIEW })
  public @interface NotificationId
  {
  }

  private Notifier(@NonNull Application context)
  {
    mContext = context;
  }

  public void notifyDownloadFailed(@Nullable String id, @Nullable String name)
  {
    String title = mContext.getString(R.string.app_name);
    String content = mContext.getString(R.string.download_country_failed, name);

    Intent intent = MwmActivity.createShowMapIntent(mContext, id)
                               .setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
    final int flags = (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M)
        ? PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_IMMUTABLE : PendingIntent.FLAG_UPDATE_CURRENT;
    PendingIntent pi = PendingIntent.getActivity(mContext, 0, intent, flags);

    String channel = NotificationChannelFactory.createProvider(mContext).getDownloadingChannel();
    placeNotification(title, content, pi, ID_DOWNLOAD_FAILED, channel);
  }

  public void cancelNotification(@NotificationId int id)
  {
    if (id == ID_NONE)
      return;

    getNotificationManager().cancel(id);
  }

  public void processNotificationExtras(@Nullable Intent intent)
  {
    if (intent == null)
      return;

    if (intent.hasExtra(Notifier.EXTRA_CANCEL_NOTIFICATION))
    {
      @Notifier.NotificationId
      int notificationId = intent.getIntExtra(Notifier.EXTRA_CANCEL_NOTIFICATION, Notifier.ID_NONE);
      cancelNotification(notificationId);
    }
  }

  private void placeNotification(String title, String content, PendingIntent pendingIntent,
                                 int notificationId, @NonNull String channel)
  {
    final Notification notification = getBuilder(title, content, pendingIntent, channel).build();

    getNotificationManager().notify(notificationId, notification);
  }

  @NonNull
  private NotificationCompat.Builder getBuilder(String title, String content,
                                                PendingIntent pendingIntent, @NonNull String channel)
  {

    return new NotificationCompat.Builder(mContext, channel)
        .setAutoCancel(true)
        .setSmallIcon(R.drawable.ic_notification)
        .setColor(UiUtils.getNotificationColor(mContext))
        .setContentTitle(title)
        .setContentText(content)
        .setTicker(getTicker(title, content))
        .setContentIntent(pendingIntent);
  }

  @NonNull
  private CharSequence getTicker(String title, String content)
  {
    int templateResId = StringUtils.isRtl() ? R.string.notification_ticker_rtl
                                            : R.string.notification_ticker_ltr;
    return mContext.getString(templateResId, title, content);
  }

  @SuppressWarnings("ConstantConditions")
  @NonNull
  private NotificationManager getNotificationManager()
  {
    return (NotificationManager) mContext.getSystemService(Context.NOTIFICATION_SERVICE);
  }

  @NonNull
  public static Notifier from(Application application)
  {
    return new Notifier(application);
  }
}
