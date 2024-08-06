package app.organicmaps.location;

import android.Manifest;
import android.app.ForegroundServiceStartNotAllowedException;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.location.Location;
import android.os.Build;
import android.os.Handler;
import android.os.IBinder;
import android.util.Log;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RequiresPermission;
import androidx.core.app.ActivityCompat;
import androidx.core.app.NotificationChannelCompat;
import androidx.core.app.NotificationCompat;
import androidx.core.app.NotificationManagerCompat;
import androidx.core.content.ContextCompat;
import app.organicmaps.MwmActivity;
import app.organicmaps.MwmApplication;
import app.organicmaps.R;
import app.organicmaps.util.LocationUtils;
import app.organicmaps.util.log.Logger;

import static android.Manifest.permission.ACCESS_FINE_LOCATION;
import static android.Manifest.permission.POST_NOTIFICATIONS;
import static android.content.pm.PackageManager.PERMISSION_GRANTED;

public class TrackRecordingService extends Service implements LocationListener
{
  public static final String TRACK_REC_CHANNEL_ID = "TRACK RECORDING";
  public static final int TRACK_REC_NOTIFICATION_ID = 54321;
  private NotificationCompat.Builder mNotificationBuilder;
  private static final String TAG = TrackRecordingService.class.getSimpleName();
  private Handler mHandler;
  private Runnable mLocationTimeoutRunnable;
  private static final int LOCATION_UPDATE_TIMEOUT = 30000;
  private boolean mWarningNotification = false;
  private NotificationCompat.Builder mWarningBuilder;
  private PendingIntent mPendingIntent;

  @Nullable
  @Override
  public IBinder onBind(Intent intent)
  {
    return null;
  }

  @RequiresPermission(value = ACCESS_FINE_LOCATION)
  public static void startForegroundService(@NonNull Context context)
  {
    if (TrackRecorder.nativeGetDuration() != 24)
      TrackRecorder.nativeSetDuration(24);
    if (!TrackRecorder.nativeIsEnabled())
      TrackRecorder.nativeSetEnabled(true);
    LocationHelper.from(context).restartWithNewMode();
    ContextCompat.startForegroundService(context, new Intent(context, TrackRecordingService.class));
  }

  public static void createNotificationChannel(@NonNull Context context)
  {
    final NotificationManagerCompat notificationManager = NotificationManagerCompat.from(context);
    final NotificationChannelCompat channel = new NotificationChannelCompat.Builder(TRACK_REC_CHANNEL_ID,
                                                                                    NotificationManagerCompat.IMPORTANCE_LOW)
        .setName(context.getString(R.string.trace_path_name))
        .setLightsEnabled(false)
        .setVibrationEnabled(false)
        .build();
    notificationManager.createNotificationChannel(channel);
  }

  private PendingIntent getPendingIntent(@NonNull Context context)
  {
    if (mPendingIntent != null)
      return mPendingIntent;

    final int FLAG_IMMUTABLE = Build.VERSION.SDK_INT < Build.VERSION_CODES.M ? 0 : PendingIntent.FLAG_IMMUTABLE;
    final Intent contentIntent = new Intent(context, MwmActivity.class);
    mPendingIntent = PendingIntent.getActivity(context, 0, contentIntent,
                                               PendingIntent.FLAG_CANCEL_CURRENT | FLAG_IMMUTABLE);
    return mPendingIntent;
  }

  @NonNull
  public NotificationCompat.Builder getNotificationBuilder(@NonNull Context context)
  {
    if (mNotificationBuilder != null)
      return mNotificationBuilder;

    mNotificationBuilder = new NotificationCompat.Builder(context, TRACK_REC_CHANNEL_ID)
        .setCategory(NotificationCompat.CATEGORY_SERVICE)
        .setPriority(NotificationManager.IMPORTANCE_DEFAULT)
        .setVisibility(NotificationCompat.VISIBILITY_SECRET)
        .setOngoing(true)
        .setShowWhen(true)
        .setOnlyAlertOnce(true)
        .setSmallIcon(R.drawable.ic_splash)
        .setContentTitle(context.getString(R.string.recent_track_recorder_running))
        .setContentText(context.getString(R.string.recent_track_rec_notif_desc))
        .setContentIntent(getPendingIntent(context))
        .setColor(ContextCompat.getColor(context, R.color.notification));

    return mNotificationBuilder;
  }

  public static void stopService(@NonNull Context context)
  {
    Logger.i(TAG);
    context.stopService(new Intent(context, TrackRecordingService.class));
  }

  @Override
  public void onDestroy()
  {
    mNotificationBuilder = null;
    mWarningBuilder = null;
    if (TrackRecorder.nativeIsEnabled())
      TrackRecorder.nativeSetEnabled(false);
    mHandler.removeCallbacks(mLocationTimeoutRunnable);
    LocationHelper.from(this).removeListener(this);
    // The notification is cancelled automatically by the system.
  }

  @Override
  public int onStartCommand(@NonNull Intent intent, int flags, int startId)
  {
    if (!MwmApplication.from(this).arePlatformAndCoreInitialized())
    {
      Logger.w(TAG, "Application is not initialized");
      stopSelf();
      return START_NOT_STICKY; // The service will be stopped by stopSelf().
    }

    if (!LocationUtils.checkFineLocationPermission(this))
    {
      // In a hypothetical scenario, the user could revoke location permissions after the app's process crashed,
      // but before the service with START_STICKY was restarted by the system.
      Logger.w(TAG, "Permission ACCESS_FINE_LOCATION is not granted, skipping TrackRecordingService");
      stopSelf();
      return START_NOT_STICKY; // The service will be stopped by stopSelf().
    }

    if (!TrackRecorder.nativeIsEnabled())
    {
      Logger.i(TAG, "Service can't be started because Track Recorder is turned off in settings");
      stopSelf();
      return START_NOT_STICKY;
    }

    Logger.i(TAG, "Starting foreground service");
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S)
    {
      try
      {
        startForeground(TrackRecordingService.TRACK_REC_NOTIFICATION_ID, getNotificationBuilder(this).build());
      } catch (ForegroundServiceStartNotAllowedException e)
      {
        Logger.e(TAG, "Oops! ForegroundService is not allowed", e);
      }
    }
    else
    {
      startForeground(TrackRecordingService.TRACK_REC_NOTIFICATION_ID, getNotificationBuilder(this).build());
    }

    final LocationHelper locationHelper = LocationHelper.from(this);

    if (mHandler == null) mHandler = new Handler();
    if (mLocationTimeoutRunnable == null) mLocationTimeoutRunnable = this::onLocationUpdateTimeout;
    mHandler.postDelayed(mLocationTimeoutRunnable, LOCATION_UPDATE_TIMEOUT);

    // Subscribe to location updates. This call is idempotent.
    locationHelper.addListener(this);

    // Restart the location with more frequent refresh interval for Track Recording.
    locationHelper.restartWithNewMode();

    return START_NOT_STICKY;
  }

  public NotificationCompat.Builder getWarningBuilder(Context context)
  {
    if (mWarningBuilder != null)
      return mWarningBuilder;

    mWarningBuilder = new NotificationCompat.Builder(context, TRACK_REC_CHANNEL_ID)
        .setCategory(NotificationCompat.CATEGORY_SERVICE)
        .setPriority(NotificationManager.IMPORTANCE_LOW)
        .setVisibility(NotificationCompat.VISIBILITY_PUBLIC)
        .setOngoing(true)
        .setShowWhen(true)
        .setOnlyAlertOnce(true)
        .setSmallIcon(R.drawable.warning_icon)
        .setContentTitle(context.getString(R.string.location_update_warning_title))
        .setContentText(context.getString(R.string.location_update_warning_desc))
        .setContentIntent(getPendingIntent(context))
        .setColor(ContextCompat.getColor(context, R.color.notification_warning));

    return mWarningBuilder;
  }

  private void onLocationUpdateTimeout()
  {
    Logger.i(TAG, "Location update timeout");
    mWarningNotification = true;
    // post notification permission is not there but we will not stop the runnable because if
    // in between user gives permission then warning will not be updated until next restart
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU &&
        ActivityCompat.checkSelfPermission(this, POST_NOTIFICATIONS) != PERMISSION_GRANTED)
      return;

    NotificationManagerCompat.from(this)
                             .notify(TRACK_REC_NOTIFICATION_ID, getWarningBuilder(this).build());
  }

  @Override
  public void onLocationUpdated(@NonNull Location location)
  {
    Logger.i(TAG, "Location is being updated in Track Recording service");
    mHandler.removeCallbacks(mLocationTimeoutRunnable);
    mHandler.postDelayed(mLocationTimeoutRunnable, LOCATION_UPDATE_TIMEOUT); // Reset the timeout.

    if (mWarningNotification)
    {
      mWarningNotification = false;

      // post notification permission is not there but we will not stop the runnable because if
      // in between user gives permission then warning will not be updated until next restart
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU &&
          ActivityCompat.checkSelfPermission(this, POST_NOTIFICATIONS) != PERMISSION_GRANTED)
        return;

      NotificationManagerCompat.from(this)
                               .notify(TRACK_REC_NOTIFICATION_ID, getNotificationBuilder(this).build());
    }
  }

  @Override
  public void onLocationDisabled()
  {
  }

}
