package app.organicmaps.routing;

import static android.Manifest.permission.ACCESS_FINE_LOCATION;
import static android.Manifest.permission.POST_NOTIFICATIONS;
import static android.content.pm.PackageManager.PERMISSION_GRANTED;

import android.app.ForegroundServiceStartNotAllowedException;
import android.app.Notification;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.drawable.Drawable;
import android.location.Location;
import android.os.Build;
import android.os.IBinder;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RequiresPermission;
import androidx.appcompat.content.res.AppCompatResources;
import androidx.core.app.ActivityCompat;
import androidx.core.app.NotificationChannelCompat;
import androidx.core.app.NotificationCompat;
import androidx.core.app.NotificationManagerCompat;
import androidx.core.content.ContextCompat;

import app.organicmaps.Framework;
import app.organicmaps.MwmActivity;
import app.organicmaps.R;
import app.organicmaps.base.MediaPlayerWrapper;
import app.organicmaps.location.LocationHelper;
import app.organicmaps.location.LocationListener;
import app.organicmaps.sound.TtsPlayer;
import app.organicmaps.util.Graphics;
import app.organicmaps.util.log.Logger;

import java.util.Objects;

public class NavigationService extends Service implements LocationListener
{
  private static final String TAG = NavigationService.class.getSimpleName();

  private static final String CHANNEL_ID = "NAVIGATION";
  private static final int NOTIFICATION_ID = 12345678;

  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private NotificationCompat.Builder mNotificationBuilder;

  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  MediaPlayerWrapper mPlayer;

  /**
   * Start the foreground service for turn-by-turn voice-guided navigation.
   *
   * @param context Context to start service from.
   */
  @RequiresPermission(value = ACCESS_FINE_LOCATION)
  public static void startForegroundService(@NonNull Context context)
  {
    Logger.i(TAG);
    ContextCompat.startForegroundService(context, new Intent(context, NavigationService.class));
  }

  /**
   * Stop the foreground service for  turn-by-turn voice-guided navigation.
   *
   * @param context Context to stop service from.
   */
  public static void stopService(@NonNull Context context)
  {
    Logger.i(TAG);
    context.stopService(new Intent(context, NavigationService.class));
  }

  /**
   * Creates notification channel for navigation.
   * @param context Context to create channel from.
   */
  public static void createNotificationChannel(@NonNull Context context)
  {
    Logger.i(TAG);

    final NotificationManagerCompat notificationManager = NotificationManagerCompat.from(context);
    final NotificationChannelCompat channel = new NotificationChannelCompat.Builder(CHANNEL_ID,
        NotificationManagerCompat.IMPORTANCE_LOW)
        .setName(context.getString(R.string.pref_navigation))
        .setLightsEnabled(false)    // less annoying
        .setVibrationEnabled(false) // less annoying
        .build();
    notificationManager.createNotificationChannel(channel);
  }

  /**
   * See {@link android.app.Notification.Builder#setColorized(boolean) }
   */
  private static boolean isColorizedSupported()
  {
    // Nice colorized notifications should be supported on API=26 and later.
    // Nonetheless, even on API=32, Xiaomi uses their own legacy implementation that displays white-on-white instead.
    return Build.VERSION.SDK_INT >= Build.VERSION_CODES.O &&
        !"xiaomi".equalsIgnoreCase(Build.MANUFACTURER);
  }

  @RequiresPermission(value = ACCESS_FINE_LOCATION)
  @Override
  public void onCreate()
  {
    Logger.i(TAG);

    /*
     * Create cached notification builder.
     */
    final int FLAG_IMMUTABLE = Build.VERSION.SDK_INT < Build.VERSION_CODES.M ? 0 : PendingIntent.FLAG_IMMUTABLE;
    final Intent contentIntent = new Intent(this, MwmActivity.class);
    final PendingIntent contentPendingIntent = PendingIntent.getActivity(this, 0, contentIntent,
        PendingIntent.FLAG_CANCEL_CURRENT | FLAG_IMMUTABLE);

    mNotificationBuilder = new NotificationCompat.Builder(this, CHANNEL_ID)
        .setCategory(NotificationCompat.CATEGORY_NAVIGATION)
        .setPriority(Notification.PRIORITY_LOW)
        .setVisibility(NotificationCompat.VISIBILITY_PUBLIC)
        .setOngoing(true)
        .setShowWhen(false)
        .setOnlyAlertOnce(true)
        .setSmallIcon(R.drawable.ic_notification)
        .setContentIntent(contentPendingIntent)
        .setColorized(isColorizedSupported())
        .setColor(ContextCompat.getColor(this, R.color.notification));

    mPlayer = new MediaPlayerWrapper(getApplicationContext());

    /*
     * Subscribe to location updates.
     */
    LocationHelper.INSTANCE.addListener(this);
  }

  @RequiresPermission(value = ACCESS_FINE_LOCATION)
  @Override
  public void onDestroy()
  {
    Logger.i(TAG);

    super.onDestroy();
    LocationHelper.INSTANCE.removeListener(this);
    TtsPlayer.INSTANCE.stop();

    final NotificationManagerCompat notificationManager = NotificationManagerCompat.from(this);
    notificationManager.cancel(NOTIFICATION_ID);

    mPlayer.release();

    // Restart the location to resubscribe with a less frequent refresh interval (see {@link onStartCommand() }).
    LocationHelper.INSTANCE.restart();
  }

  @Override
  public void onLowMemory()
  {
    super.onLowMemory();
    Logger.d(TAG, "onLowMemory()");
  }

  @RequiresPermission(value = ACCESS_FINE_LOCATION)
  @Override
  public int onStartCommand(Intent intent, int flags, int startId)
  {
    Logger.i(TAG, "Starting foreground");
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S)
    {
      try
      {
        startForeground(NavigationService.NOTIFICATION_ID, mNotificationBuilder.build());
      }
      catch (ForegroundServiceStartNotAllowedException e)
      {
        Logger.e(TAG, "Oops! ForegroundService is not allowed", e);
      }
    }
    else
    {
      startForeground(NavigationService.NOTIFICATION_ID, mNotificationBuilder.build());
    }

    // Tests on different devices demonstrated that background location works significantly longer when
    // requested AFTER starting the foreground service. Restarting the location is also necessary to
    // re-subscribe for more frequent GPS updates for navigation.
    LocationHelper.INSTANCE.restart();

    return START_STICKY;
  }

  @Nullable
  @Override
  public IBinder onBind(Intent intent)
  {
    Logger.i(TAG);
    return null;
  }

  @Override
  public void onLocationUpdated(@NonNull Location location)
  {
    // Ignore any pending notifications when service is being stopping.
    final RoutingController routingController = RoutingController.get();
    if (!routingController.isNavigating())
      return;

    // TODO: consider to create callback mechanism to transfer 'ROUTE_IS_FINISHED' event from
    // the core to the platform code (https://github.com/organicmaps/organicmaps/issues/3589),
    // because calling the native method 'nativeIsRouteFinished'
    // too often can result in poor UI performance.
    if (Framework.nativeIsRouteFinished())
      routingController.cancel();

    final String[] turnNotifications = Framework.nativeGenerateNotifications();
    if (turnNotifications != null)
      TtsPlayer.INSTANCE.playTurnNotifications(getApplicationContext(), turnNotifications);

    final RoutingInfo routingInfo = Framework.nativeGetRouteFollowingInfo();
    if (routingInfo == null)
      return;

    if (routingInfo.shouldPlayWarningSignal())
      mPlayer.playback(R.raw.speed_cams_beep);

    // Don't spend time on updating RemoteView if notifications are not allowed.
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU &&
        ActivityCompat.checkSelfPermission(this, POST_NOTIFICATIONS) != PERMISSION_GRANTED)
      return;

    final Drawable drawable = Objects.requireNonNull(AppCompatResources.getDrawable(this,
        routingInfo.carDirection.getTurnRes()));
    final Bitmap bitmap = isColorizedSupported() ?
        Graphics.drawableToBitmap(drawable) :
        Graphics.drawableToBitmapWithTint(drawable, ContextCompat.getColor(this, R.color.base_accent));

    final Notification notification = mNotificationBuilder
        .setLargeIcon(bitmap)
        .setContentTitle(routingInfo.distToTurn.toString(this))
        .setContentText(routingInfo.nextStreet)
        .build();

    // The notification object must be re-created for every update.
    final NotificationManagerCompat notificationManager = NotificationManagerCompat.from(this);
    notificationManager.notify(NOTIFICATION_ID, notification);
  }
}
