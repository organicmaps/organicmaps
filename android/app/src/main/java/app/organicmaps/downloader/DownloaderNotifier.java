package app.organicmaps.downloader;

import static android.Manifest.permission.POST_NOTIFICATIONS;
import static android.content.pm.PackageManager.PERMISSION_GRANTED;

import android.app.Notification;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.os.Build;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StringRes;
import androidx.core.app.NotificationChannelCompat;
import androidx.core.app.NotificationCompat;
import androidx.core.app.NotificationManagerCompat;
import androidx.core.content.ContextCompat;

import app.organicmaps.MwmActivity;
import app.organicmaps.R;
import app.organicmaps.util.StringUtils;
import app.organicmaps.util.log.Logger;

public class DownloaderNotifier
{
  private static final String TAG = DownloaderNotifier.class.getSimpleName();

  private static final String CHANNEL_ID = "downloader";
  public static final int NOTIFICATION_ID = 1;

  private final Context mContext;
  private NotificationCompat.Builder mProgressNotificationBuilder;

  public DownloaderNotifier(Context context)
  {
    mContext = context;
  }

  public static void createNotificationChannel(@NonNull Context context)
  {
    final NotificationManagerCompat notificationManager = NotificationManagerCompat.from(context);
    final NotificationChannelCompat channel = new NotificationChannelCompat.Builder(CHANNEL_ID,
        NotificationManagerCompat.IMPORTANCE_LOW)
        .setName(context.getString(R.string.notification_channel_downloader))
        .setShowBadge(true)
        .setVibrationEnabled(false)
        .setLightsEnabled(false)
        .build();
    notificationManager.createNotificationChannel(channel);
  }

  public void notifyDownloadFailed(@Nullable String countryId)
  {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU &&
        ContextCompat.checkSelfPermission(mContext, POST_NOTIFICATIONS) != PERMISSION_GRANTED)
    {
      Logger.w(TAG, "Permission POST_NOTIFICATIONS is not granted, skipping notification");
      return;
    }

    final String title = mContext.getString(R.string.app_name);
    final String countryName = MapManager.nativeGetName(countryId);
    final String content = mContext.getString(R.string.download_country_failed, countryName);

    var contentPendingIntent = getNotificationPendingIntent(countryId);

    final Notification notification = new NotificationCompat.Builder(mContext, CHANNEL_ID)
        .setAutoCancel(true)
        .setCategory(NotificationCompat.CATEGORY_ERROR)
        .setVisibility(NotificationCompat.VISIBILITY_PUBLIC)
        .setSmallIcon(R.drawable.ic_splash)
        .setColor(ContextCompat.getColor(mContext, R.color.notification))
        .setContentTitle(title)
        .setContentText(content)
        .setShowWhen(true)
        .setTicker(getTicker(mContext, title, content))
        .setContentIntent(contentPendingIntent)
        .setOnlyAlertOnce(true)
        .build();

    Logger.i(TAG, "Notifying about failed map download");
    final NotificationManagerCompat notificationManager = NotificationManagerCompat.from(mContext);
    notificationManager.notify(NOTIFICATION_ID, notification);
  }

  public void notifyProgress() {
    notifyProgress(null, 0, 0);
  }

  public void notifyProgress(@Nullable String countryId, int maxProgress, int progress) {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU &&
            ContextCompat.checkSelfPermission(mContext, POST_NOTIFICATIONS) != PERMISSION_GRANTED)
    {
      Logger.w(TAG, "Permission POST_NOTIFICATIONS is not granted, skipping notification");
      return;
    }

    NotificationManagerCompat.from(mContext).notify(NOTIFICATION_ID, buildProgressNotification(countryId, maxProgress, progress));
  }

  @NonNull
  public Notification buildProgressNotification()
  {
    return buildProgressNotification(null, 0, 0);
  }

  @NonNull
  public Notification buildProgressNotification(@Nullable String countryId, int maxProgress, int progress)
  {
    var builder = startNotification(countryId);

    builder.setProgress(maxProgress, progress, maxProgress == 0);
    builder.setContentText("Download in progress");

    return builder.build();
  }

  @NonNull
  private NotificationCompat.Builder startNotification(@Nullable String countryId)
  {
    final String title = mContext.getString(R.string.app_name);

    return new NotificationCompat.Builder(mContext, CHANNEL_ID)
            .setAutoCancel(true)
            .setVisibility(NotificationCompat.VISIBILITY_PUBLIC)
            .setSmallIcon(R.drawable.ic_splash)
            .setColor(ContextCompat.getColor(mContext, R.color.notification))
            .setShowWhen(true)
            .setContentTitle(title)
            .setContentIntent(getNotificationPendingIntent(countryId));
  }

  @NonNull
  private PendingIntent getNotificationPendingIntent(@Nullable String countryId) {
    final int FLAG_IMMUTABLE = Build.VERSION.SDK_INT < Build.VERSION_CODES.M ? 0 : PendingIntent.FLAG_IMMUTABLE;
    final Intent contentIntent = MwmActivity.createShowMapIntent(mContext, countryId);
    contentIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
    return PendingIntent.getActivity(mContext, 0, contentIntent, PendingIntent.FLAG_UPDATE_CURRENT | FLAG_IMMUTABLE);
  }

  @NonNull
  private static CharSequence getTicker(@NonNull Context context, @NonNull String title, @NonNull String content)
  {
    @StringRes final int templateResId = StringUtils.isRtl() ? R.string.notification_ticker_rtl
        : R.string.notification_ticker_ltr;
    return context.getString(templateResId, title, content);
  }
}
