package app.organicmaps.routing;

import static android.Manifest.permission.ACCESS_COARSE_LOCATION;
import static android.Manifest.permission.ACCESS_FINE_LOCATION;
import static android.Manifest.permission.POST_NOTIFICATIONS;
import static android.app.PendingIntent.FLAG_IMMUTABLE;
import static android.app.PendingIntent.FLAG_UPDATE_CURRENT;
import static android.content.pm.PackageManager.PERMISSION_GRANTED;
import static app.organicmaps.sdk.util.Constants.Vendor.XIAOMI;

import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.pm.ServiceInfo;
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
import androidx.core.app.ServiceCompat;
import androidx.core.content.ContextCompat;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.OrganicMaps;
import app.organicmaps.sdk.location.LocationHelper;
import app.organicmaps.sdk.location.LocationListener;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.routing.RoutingInfo;
import app.organicmaps.sdk.sound.MediaPlayerWrapper;
import app.organicmaps.sdk.sound.TtsPlayer;
import app.organicmaps.sdk.util.Assert;
import app.organicmaps.sdk.util.Config;
import app.organicmaps.sdk.util.Graphics;
import app.organicmaps.sdk.util.LocationUtils;
import app.organicmaps.sdk.util.log.Logger;

public class NavigationService extends Service implements LocationListener
{
  public class Binder extends android.os.Binder
  {
    public void setOrganicMapsContext(@NonNull OrganicMaps organicMapsContext)
    {
      mOrganicMapsContext = organicMapsContext;
      mLocationHelper = mOrganicMapsContext.getLocationHelper();
      mLocationHelper.addListener(NavigationService.this);
      if (LocationUtils.checkFineLocationPermission(mOrganicMapsContext.getApplicationContext()))
        mLocationHelper.restartWithNewMode();
      else
        Assert.debug(false, "Permission ACCESS_FINE_LOCATION is not granted");
    }

    public void setContentIntent(@NonNull PendingIntent intent)
    {
      mContentIntent = intent;
      prepareNotificationBuilder();
    }

    public void setCarNotificationExtender(@NonNull NotificationCompat.Extender notificationExtender)
    {
      mCarNotificationExtender = notificationExtender;
    }
  }

  private static final String TAG = NavigationService.class.getSimpleName();
  private static final String STOP_NAVIGATION = "STOP_NAVIGATION";
  private static final String CHANNEL_ID = "NAVIGATION";
  private static final int NOTIFICATION_ID = 12345678;

  @NonNull
  private final IBinder mBinder = new Binder();
  @Nullable
  private OrganicMaps mOrganicMapsContext;
  @Nullable
  private LocationHelper mLocationHelper;
  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private MediaPlayerWrapper mPlayer;
  @Nullable
  private PendingIntent mContentIntent;
  @Nullable
  private NotificationCompat.Builder mNotificationBuilder;
  @Nullable
  private NotificationCompat.Extender mCarNotificationExtender;

  /**
   * Creates notification channel for navigation.
   *
   * @param context Context to create channel from.
   */
  public static void createNotificationChannel(@NonNull Context context)
  {
    Logger.i(TAG);

    final NotificationManagerCompat notificationManager = NotificationManagerCompat.from(context);
    final NotificationChannelCompat channel =
        new NotificationChannelCompat.Builder(CHANNEL_ID,
                                              NotificationManagerCompat.IMPORTANCE_LOW)
            .setName(context.getString(R.string.navigation_channel_name))
            .setLightsEnabled(false) // less annoying
            .setVibrationEnabled(false) // less annoying
            .build();
    notificationManager.createNotificationChannel(channel);
  }

  /**
   * Start the foreground service for turn-by-turn voice-guided navigation.
   *
   * @param context Context to start service from.
   * @param serviceConnection Service connection to bind to.
   */
  @RequiresPermission(value = ACCESS_FINE_LOCATION)
  public static void startService(@NonNull Context context, @NonNull ServiceConnection serviceConnection)
  {
    Logger.i(TAG);
    ContextCompat.startForegroundService(context, new Intent(context, NavigationService.class));
    context.bindService(new Intent(context, NavigationService.class), serviceConnection, Context.BIND_AUTO_CREATE);
  }

  /**
   * Stop the foreground service for turn-by-turn voice-guided navigation.
   *
   * @param context Context to stop service from.
   * @param serviceConnection Service connection to unbind from.
   */
  public static void stopService(@NonNull Context context, @NonNull ServiceConnection serviceConnection)
  {
    Logger.i(TAG);
    context.unbindService(serviceConnection);
    context.stopService(new Intent(context, NavigationService.class));
  }

  @Nullable
  @Override
  public IBinder onBind(@Nullable Intent intent)
  {
    return mBinder;
  }

  @Override
  public int onStartCommand(@NonNull Intent intent, int flags, int startId)
  {
    if (STOP_NAVIGATION.equals(intent.getAction()))
    {
      RoutingController.get().cancel();
      stopSelf();
    }

    if (mOrganicMapsContext != null && !mOrganicMapsContext.arePlatformAndCoreInitialized())
    {
      // The system restarts the service if the app's process has crashed or been stopped. It would be nice to
      // automatically restore the last route and resume navigation. Unfortunately, the current implementation of
      // the routing state machine (RoutingController and underlying NDK part) requires a complete re-planning of
      // the route. Such operation can fail for some reason. We have no UI (i.e. RoutePlanFragment) started to
      // handle any route planning errors. Starting any new Activities from Services is not allowed also.
      // https://github.com/organicmaps/organicmaps/issues/6233
      Logger.w(TAG, "Application is not initialized");
      stopSelf();
      return START_NOT_STICKY; // The service will be stopped by stopSelf().
    }

    if (!LocationUtils.checkFineLocationPermission(this))
    {
      // In a hypothetical scenario, the user could revoke location permissions after the app's process crashed,
      // but before the service with START_STICKY was restarted by the system.
      Logger.w(TAG, "Permission ACCESS_FINE_LOCATION is not granted, skipping NavigationService");
      stopSelf();
      return START_NOT_STICKY; // The service will be stopped by stopSelf().
    }

    Assert.debug(mNotificationBuilder != null, "Notification builder is null");
    try
    {
      final int fgsType = Build.VERSION.SDK_INT >= 29 ? ServiceInfo.FOREGROUND_SERVICE_TYPE_LOCATION
                                                            | ServiceInfo.FOREGROUND_SERVICE_TYPE_MEDIA_PLAYBACK
                                                      : 0;
      ServiceCompat.startForeground(this, NOTIFICATION_ID, mNotificationBuilder.build(), fgsType);
    }
    catch (SecurityException e)
    {
      // It is unknown why, but on Android 14+ devices starting services fails despite permission checks above.
      Logger.e(TAG, "Failed to start foreground service, stopping the service", e);
      stopSelf();
      return START_NOT_STICKY;
    }

    // Please make this service START_STICKY after fixing the issues at the beginning of the function.
    return START_NOT_STICKY;
  }

  @Override
  public void onLowMemory()
  {
    Logger.d(TAG);
  }

  @Override
  public void onCreate()
  {
    Logger.i(TAG);

    mPlayer = new MediaPlayerWrapper(getApplicationContext());

    prepareNotificationBuilder();
  }

  @Override
  public void onDestroy()
  {
    Logger.i(TAG);

    if (mLocationHelper != null)
    {
      mLocationHelper.removeListener(this);
      mLocationHelper = null;
    }
    mNotificationBuilder = null;
    mCarNotificationExtender = null;
    TtsPlayer.INSTANCE.stop();
    mPlayer.release();
  }

  @Override
  @RequiresPermission(anyOf = {ACCESS_COARSE_LOCATION, ACCESS_FINE_LOCATION})
  public void onLocationUpdated(@NonNull Location location)
  {
    // Ignore any pending notifications when the service is being stopped.
    final RoutingController routingController = RoutingController.get();
    if (!routingController.isNavigating())
      return;

    // Voice the turn notification first.
    final String[] turnNotifications = Framework.nativeGenerateNotifications(Config.TTS.getAnnounceStreets());
    if (turnNotifications != null)
      TtsPlayer.INSTANCE.playTurnNotifications(turnNotifications);

    // TODO: consider to create callback mechanism to transfer 'ROUTE_IS_FINISHED' event from
    // the core to the platform code (https://github.com/organicmaps/organicmaps/issues/3589),
    // because calling the native method 'nativeIsRouteFinished'
    // too often can result in poor UI performance.
    // This check should be done after playTurnNotifications() to play the last turn notification.
    if (Framework.nativeIsRouteFinished())
    {
      routingController.cancel();
      if (mLocationHelper != null)
        mLocationHelper.restartWithNewMode();
      stopSelf();
      return;
    }

    final RoutingInfo routingInfo = Framework.nativeGetRouteFollowingInfo();
    if (routingInfo == null)
      return;

    if (routingInfo.shouldPlayWarningSignal())
      mPlayer.playback(R.raw.speed_cams_beep);

    // Don't spend time on updating RemoteView if notifications are not allowed.
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU
        && ActivityCompat.checkSelfPermission(this, POST_NOTIFICATIONS) != PERMISSION_GRANTED)
      return;

    Assert.debug(mNotificationBuilder != null, "Notification builder is null");
    final NotificationCompat.Builder notificationBuilder =
        mNotificationBuilder.setContentTitle(routingInfo.distToTurn.toString(this))
            .setContentText(routingInfo.nextStreet);

    final Drawable drawable =
        AppCompatResources.getDrawable(this, routingInfo.carDirection.getTurnRes(routingInfo.exitNum));
    if (drawable != null)
    {
      final Bitmap bitmap =
          isColorizedSupported()
              ? Graphics.drawableToBitmap(drawable)
              : Graphics.drawableToBitmapWithTint(
                    drawable, ContextCompat.getColor(this, app.organicmaps.branding.R.color.base_accent));
      notificationBuilder.setLargeIcon(bitmap);
    }

    if (mCarNotificationExtender != null)
      notificationBuilder.extend(mCarNotificationExtender);

    // The notification object must be re-created for every update.
    NotificationManagerCompat.from(this).notify(NOTIFICATION_ID, notificationBuilder.build());
  }

  private void prepareNotificationBuilder()
  {
    final Context context = getApplicationContext();

    final Intent exitIntent = new Intent(context, NavigationService.class);
    exitIntent.setAction(STOP_NAVIGATION);
    final PendingIntent exitPendingIntent =
        PendingIntent.getService(context, 0, exitIntent, FLAG_UPDATE_CURRENT | FLAG_IMMUTABLE);

    mNotificationBuilder = new NotificationCompat.Builder(context, CHANNEL_ID)
                               .setCategory(NotificationCompat.CATEGORY_NAVIGATION)
                               .setVisibility(NotificationCompat.VISIBILITY_PUBLIC)
                               .setOngoing(true)
                               .setShowWhen(false)
                               .setOnlyAlertOnce(true)
                               .setSmallIcon(app.organicmaps.branding.R.drawable.ic_splash)
                               .addAction(0, context.getString(R.string.navigation_stop_button), exitPendingIntent)
                               .setColorized(isColorizedSupported())
                               .setColor(ContextCompat.getColor(context, R.color.notification));

    if (Build.VERSION.SDK_INT >= 24)
      mNotificationBuilder.setPriority(NotificationManager.IMPORTANCE_LOW);

    if (mContentIntent != null)
      mNotificationBuilder.setContentIntent(mContentIntent);
  }

  /**
   * See {@link android.app.Notification.Builder#setColorized(boolean) }
   */
  private static boolean isColorizedSupported()
  {
    // Nice colorized notifications should be supported on API=26 and later.
    // Nonetheless, even on API=32, Xiaomi uses their own legacy implementation that displays white-on-white instead.
    return Build.VERSION.SDK_INT >= Build.VERSION_CODES.O && !XIAOMI.equalsIgnoreCase(Build.MANUFACTURER);
  }
}
