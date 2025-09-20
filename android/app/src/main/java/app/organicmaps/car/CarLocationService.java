package app.organicmaps.car;

import static android.Manifest.permission.ACCESS_FINE_LOCATION;

import android.app.*;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ServiceInfo;
import android.os.Build;
import android.os.IBinder;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RequiresPermission;
import androidx.core.app.NotificationCompat;
import androidx.core.content.ContextCompat;
import app.organicmaps.R;
import app.organicmaps.sdk.util.LocationUtils;
import app.organicmaps.sdk.util.log.Logger;

public class CarLocationService extends Service
{
  private static final int NOTIFICATION_ID = 13243546;

  private static final String TAG = CarLocationService.class.getSimpleName();

  /**
   * Start the foreground service.
   *
   * @param context Context to start service from.
   */
  @RequiresPermission(value = ACCESS_FINE_LOCATION)
  public static void start(@NonNull Context context)
  {
    Logger.i(TAG);
    ContextCompat.startForegroundService(context, new Intent(context, CarLocationService.class));
  }

  /**
   * Stop service
   *
   * @param context Context to stop service from.
   */
  public static void stop(@NonNull Context context)
  {
    Logger.i(TAG);
    context.stopService(new Intent(context, CarLocationService.class));
  }

  @Override
  public int onStartCommand(Intent intent, int flags, int startId)
  {
    if (!LocationUtils.checkFineLocationPermission(this))
    {
      stopSelf();
      return START_NOT_STICKY; // The service will be stopped by stopSelf().
    }

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q)
      startForeground(CarLocationService.NOTIFICATION_ID, getNotification(this),
                      ServiceInfo.FOREGROUND_SERVICE_TYPE_LOCATION);
    else
      startForeground(CarLocationService.NOTIFICATION_ID, getNotification(this));
    return START_NOT_STICKY;
  }

  @NonNull
  private static Notification getNotification(@NonNull Context context)
  {
    return new NotificationCompat.Builder(context, CarAppService.ANDROID_AUTO_NOTIFICATION_CHANNEL_ID)
        .setCategory(NotificationCompat.CATEGORY_NAVIGATION)
        .setPriority(NotificationManager.IMPORTANCE_LOW)
        .setVisibility(NotificationCompat.VISIBILITY_PUBLIC)
        .setOngoing(true)
        .setShowWhen(false)
        .setOnlyAlertOnce(true)
        .setSmallIcon(R.drawable.ic_splash)
        .setColor(ContextCompat.getColor(context, R.color.notification))
        .setContentTitle("Android Auto")
        .setContentText(context.getString(R.string.car_used_on_the_car_screen))
        .build();
  }

  @Nullable
  @Override
  public IBinder onBind(Intent intent)
  {
    return null;
  }
}
