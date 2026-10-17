package app.organicmaps.location;

import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ServiceInfo;
import android.os.Build;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.SystemClock;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.app.NotificationChannelCompat;
import androidx.core.app.NotificationCompat;
import androidx.core.app.NotificationManagerCompat;
import androidx.core.app.ServiceCompat;
import androidx.core.content.ContextCompat;
import app.organicmaps.MwmActivity;
import app.organicmaps.MwmApplication;
import app.organicmaps.R;
import app.organicmaps.sdk.location.NoGps;
import app.organicmaps.sdk.util.log.Logger;

/**
 * Keeps the navigation without GPS and the link to the box running while the app is in the background and the box
 * tells the car data: without it Android stops the app, and the car drives on uncounted. On October 16 a car switched
 * away from the app for 6 s at 9 km/h was shown 20 m behind. A car parked, the box tells no car data: the service
 * stops, and the navigation with it, as without the service.
 */
public class NoGpsBoxService extends Service
{
  private static final String TAG = NoGpsBoxService.class.getSimpleName();
  public static final String CHANNEL_ID = "NOGPS BOX";
  private static final int NOTIFICATION_ID = 54322;
  private static final long CHECK_INTERVAL_MS = 10_000;
  // The box loses the car for a few seconds now and then while it drives, e.g. when the adapter reconnects.
  private static final long NO_CAR_DATA_STOP_MS = 60_000;

  private final Handler mHandler = new Handler(Looper.getMainLooper());
  private long mCarDataMs;
  private final Runnable mCheck = this::checkCarData;

  /**
   * Starts the service if the box tells the car data now.
   *
   * @return false if the car is not followed by the box, or the system doesn't let the service start.
   */
  public static boolean startIfCarData(@NonNull Context context)
  {
    if (!NoGps.getStatus().hasCarData())
      return false;
    try
    {
      ContextCompat.startForegroundService(context, new Intent(context, NoGpsBoxService.class));
      return true;
    }
    catch (IllegalStateException e)
    {
      // Android 12+ doesn't start a foreground service of an app in the background but when it leaves the screen.
      Logger.w(TAG, "The service is not started", e);
      return false;
    }
  }

  public static void stop(@NonNull Context context)
  {
    context.stopService(new Intent(context, NoGpsBoxService.class));
  }

  public static void createNotificationChannel(@NonNull Context context)
  {
    final NotificationChannelCompat channel =
        new NotificationChannelCompat.Builder(CHANNEL_ID, NotificationManagerCompat.IMPORTANCE_LOW)
            .setName(context.getString(R.string.nogps_background_channel))
            .setLightsEnabled(false)
            .setVibrationEnabled(false)
            .build();
    NotificationManagerCompat.from(context).createNotificationChannel(channel);
  }

  @Nullable
  @Override
  public IBinder onBind(Intent intent)
  {
    return null;
  }

  @Override
  public int onStartCommand(@Nullable Intent intent, int flags, int startId)
  {
    if (!MwmApplication.from(this).getOrganicMaps().arePlatformAndCoreInitialized())
    {
      // Android re-delivers a start command that was in flight when the process died.
      Logger.w(TAG, "Application is not initialized");
      stopSelf();
      return START_NOT_STICKY;
    }
    final int FLAG_IMMUTABLE = Build.VERSION.SDK_INT < Build.VERSION_CODES.M ? 0 : PendingIntent.FLAG_IMMUTABLE;
    final PendingIntent contentIntent = PendingIntent.getActivity(this, 0, new Intent(this, MwmActivity.class),
                                                                  PendingIntent.FLAG_UPDATE_CURRENT | FLAG_IMMUTABLE);
    final NotificationCompat.Builder notification = new NotificationCompat.Builder(this, CHANNEL_ID)
                                                        .setCategory(NotificationCompat.CATEGORY_SERVICE)
                                                        .setPriority(NotificationCompat.PRIORITY_LOW)
                                                        .setVisibility(NotificationCompat.VISIBILITY_SECRET)
                                                        .setOngoing(true)
                                                        .setShowWhen(false)
                                                        .setOnlyAlertOnce(true)
                                                        .setSmallIcon(app.organicmaps.branding.R.drawable.ic_splash)
                                                        .setContentTitle(getString(R.string.nogps_background_following))
                                                        .setContentIntent(contentIntent)
                                                        .setColor(ContextCompat.getColor(this, R.color.notification));
    try
    {
      ServiceCompat.startForeground(
          this, NOTIFICATION_ID, notification.build(),
          Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q ? ServiceInfo.FOREGROUND_SERVICE_TYPE_CONNECTED_DEVICE : 0);
    }
    catch (RuntimeException e)
    {
      // Android 14+ requires the Bluetooth permission for a service of a connected device.
      Logger.w(TAG, "The service can't run in the foreground", e);
      stopSelf();
      MwmApplication.from(this).onCarNotFollowedInBackground();
      return START_NOT_STICKY;
    }
    Logger.i(TAG, "The car is followed by the box in the background");
    mCarDataMs = SystemClock.elapsedRealtime();
    mHandler.removeCallbacks(mCheck);
    mHandler.postDelayed(mCheck, CHECK_INTERVAL_MS);
    return START_NOT_STICKY;
  }

  private void checkCarData()
  {
    final long now = SystemClock.elapsedRealtime();
    if (NoGps.getStatus().hasCarData())
      mCarDataMs = now;
    else if (now - mCarDataMs >= NO_CAR_DATA_STOP_MS)
    {
      Logger.i(TAG, "No car data for " + (now - mCarDataMs) / 1000 + " s: the car is parked");
      stopSelf();
      MwmApplication.from(this).onCarNotFollowedInBackground();
      return;
    }
    mHandler.postDelayed(mCheck, CHECK_INTERVAL_MS);
  }

  @Override
  public void onDestroy()
  {
    Logger.d(TAG);
    mHandler.removeCallbacks(mCheck);
    super.onDestroy();
  }
}
