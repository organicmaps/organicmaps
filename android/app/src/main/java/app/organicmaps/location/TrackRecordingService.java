package app.organicmaps.location;

import android.app.ForegroundServiceStartNotAllowedException;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.location.Location;
import android.os.Build;
import android.os.IBinder;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RequiresPermission;
import androidx.core.app.NotificationChannelCompat;
import androidx.core.app.NotificationCompat;
import androidx.core.app.NotificationManagerCompat;
import androidx.core.content.ContextCompat;
import app.organicmaps.MwmActivity;
import app.organicmaps.MwmApplication;
import app.organicmaps.R;
import app.organicmaps.util.Config;
import app.organicmaps.util.LocationUtils;
import app.organicmaps.util.log.Logger;

import static android.Manifest.permission.ACCESS_FINE_LOCATION;

public class TrackRecordingService extends Service implements LocationListener
{
  public static final String TRACK_REC_CHANNEL_ID = "TRACK RECORDING";
  public static final int TRACK_REC_NOTIFICATION_ID = 54321;
  private static NotificationCompat.Builder mNotificationBuilder;
  private static final String TAG = TrackRecordingService.class.getSimpleName();

  @Nullable
  @Override
  public IBinder onBind(Intent intent)
  {
    return null;
  }
  @RequiresPermission(value = ACCESS_FINE_LOCATION)
  public static void startForegroundService(@NonNull Context context)
  {
    if (!TrackRecorder.nativeIsEnabled()) TrackRecorder.nativeSetEnabled(true);
    TrackRecorder.nativeSetDuration(Config.getRecentTrackRecorderDuration());
    ContextCompat.startForegroundService(context, new Intent(context, TrackRecordingService.class));
  }

  public static void createNotificationChannel(@NonNull Context context)
  {
    final NotificationManagerCompat notificationManager = NotificationManagerCompat.from(context);
    final NotificationChannelCompat channel = new NotificationChannelCompat.Builder(TRACK_REC_CHANNEL_ID,
                                                                                    NotificationManagerCompat.IMPORTANCE_LOW)
        .setName(context.getString(R.string.recent_track_recording))
        .setLightsEnabled(false)
        .setVibrationEnabled(false)
        .build();
    notificationManager.createNotificationChannel(channel);
  }


  @NonNull
  public static NotificationCompat.Builder getNotificationBuilder(@NonNull Context context)
  {
    if (mNotificationBuilder != null)
      return mNotificationBuilder;

    final int FLAG_IMMUTABLE = Build.VERSION.SDK_INT < Build.VERSION_CODES.M ? 0 : PendingIntent.FLAG_IMMUTABLE;
    final Intent contentIntent = new Intent(context, MwmActivity.class);
    final PendingIntent pendingIntent = PendingIntent.getActivity(context, 0, contentIntent,
                                                                  PendingIntent.FLAG_CANCEL_CURRENT | FLAG_IMMUTABLE);

    mNotificationBuilder = new NotificationCompat.Builder(context, TRACK_REC_CHANNEL_ID)
        .setCategory(NotificationCompat.CATEGORY_SERVICE)
        .setPriority(NotificationManager.IMPORTANCE_DEFAULT)
        .setVisibility(NotificationCompat.VISIBILITY_PUBLIC)
        .setOngoing(true)
        .setShowWhen(true)
        .setOnlyAlertOnce(true)
        .setSmallIcon(R.drawable.ic_splash)
        .setContentTitle(context.getString(R.string.recent_track_recorder_running))
        .setContentText(context.getString(R.string.recent_track_rec_notif_desc))
        .setContentIntent(pendingIntent)
        .setColor(ContextCompat.getColor(context, R.color.notification));

    return mNotificationBuilder;
  }

  public static void stopService(@NonNull Context context)
  {
    Logger.i(TAG);
    if (TrackRecorder.nativeIsEnabled()) TrackRecorder.nativeSetEnabled(false);
    context.stopService(new Intent(context, TrackRecordingService.class));
  }

  @Override
  public void onDestroy()
  {
    mNotificationBuilder = null;
    LocationHelper.from(this).removeListener(this);
    if(TrackRecorder.nativeIsEnabled()) TrackRecorder.nativeSetEnabled(false);
    Config.setRecentTrackRecorderState(false);
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

    if(!TrackRecorder.nativeIsEnabled())
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

    // Subscribe to location updates. This call is idempotent.
    locationHelper.addListener(this);

    // Restart the location with more frequent refresh interval for Track Recording.
    locationHelper.restartWithNewMode();

    return START_NOT_STICKY;
  }

  @Override
  public void onLocationUpdated(@NonNull Location location)
  {
    Logger.i(TAG,"Location is being updated in Track Recording service");
  }

  @Override
  public void onLocationDisabled()
  {
    TrackRecorder.getInstance().stopTrackRecording();
    stopSelf();
  }

}
