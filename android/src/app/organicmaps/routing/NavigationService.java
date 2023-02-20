package app.organicmaps.routing;

import static androidx.core.app.NotificationCompat.Builder;

import android.app.ActivityManager;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.res.Configuration;
import android.location.Location;
import android.os.Binder;
import android.os.Build;
import android.os.IBinder;
import android.widget.RemoteViews;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.app.NotificationCompat;

import app.organicmaps.Framework;
import app.organicmaps.MwmActivity;
import app.organicmaps.R;
import app.organicmaps.location.LocationHelper;
import app.organicmaps.location.LocationListener;
import app.organicmaps.sound.TtsPlayer;
import app.organicmaps.util.StringUtils;
import app.organicmaps.util.Utils;
import app.organicmaps.util.log.Logger;

public class NavigationService extends Service
{
  private static final String TAG = NavigationService.class.getSimpleName();

  public static final String PACKAGE_NAME = NavigationService.class.getPackage().getName();
  public static final String PACKAGE_NAME_WITH_SERVICE_NAME = PACKAGE_NAME + "." +
      StringUtils.toLowerCase(NavigationService.class.getSimpleName());
  private static final String EXTRA_STOP_SERVICE = PACKAGE_NAME_WITH_SERVICE_NAME + "finish";

  private static final String CHANNEL_ID = "LOCATION_CHANNEL";
  private static final int NOTIFICATION_ID = 12345678;

  @NonNull
  private final IBinder mBinder = new LocalBinder();
  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private String mNavigationText = "";
  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private RemoteViews mRemoteViews;

  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private NotificationManager mNotificationManager;

  private boolean mChangingConfiguration = false;

  @NonNull
  private final LocationListener mLocationListener = new LocationListener.Simple()
  {
    @Override
    public void onLocationUpdated(Location location)
    {
      Logger.d(TAG, "onLocationUpdated()");
      RoutingInfo routingInfo = Framework.nativeGetRouteFollowingInfo();
      if (serviceIsRunningInForeground(getApplicationContext()))
      {
        mNotificationManager.notify(NOTIFICATION_ID, getNotification());
        updateNotification(routingInfo);
      }
    }
  };

  public class LocalBinder extends Binder
  {
    NavigationService getService()
    {
      return NavigationService.this;
    }
  }

  @Override
  public void onCreate()
  {
    mNotificationManager = (NotificationManager) getSystemService(NOTIFICATION_SERVICE);
    mRemoteViews = new RemoteViews(getPackageName(), R.layout.navigation_notification);

    // Android O requires a Notification Channel.
    if (Utils.isOreoOrLater())
    {
      CharSequence name = getString(R.string.app_name);
      // Create the channel for the notification
      NotificationChannel mChannel =
          new NotificationChannel(CHANNEL_ID, name, NotificationManager.IMPORTANCE_DEFAULT);

      mNotificationManager.createNotificationChannel(mChannel);
    }
  }

  @Override
  public void onDestroy()
  {
    super.onDestroy();
    removeLocationUpdates();
  }

  @Override
  public void onLowMemory()
  {
    super.onLowMemory();
    Logger.d(TAG, "onLowMemory()");
  }

  @Override
  public int onStartCommand(Intent intent, int flags, int startId)
  {
    Logger.i(TAG, "Service started");
    LocationHelper.INSTANCE.addListener(mLocationListener);
    boolean finishedFromNotification = intent.getBooleanExtra(EXTRA_STOP_SERVICE,
                                                              false);
    // We got here because the user decided to remove location updates from the notification.
    if (finishedFromNotification)
    {
      stopForeground(true);
      removeLocationUpdates();
    }
    return START_NOT_STICKY;
  }

  @Override
  public void onConfigurationChanged(Configuration newConfig)
  {
    super.onConfigurationChanged(newConfig);
    mChangingConfiguration = true;
  }

  @Override
  public IBinder onBind(Intent intent)
  {
    Logger.i(TAG, "in onBind()");
    stopForeground(true);
    mChangingConfiguration = false;
    return mBinder;
  }

  @Override
  public void onRebind(Intent intent)
  {
    Logger.i(TAG, "in onRebind()");
    stopForeground(true);
    mChangingConfiguration = false;
    super.onRebind(intent);
  }

  @Override
  public boolean onUnbind(Intent intent)
  {
    Logger.i(TAG, "Last client unbound from service");

    // Called when the last client unbinds from this
    // service. If this method is called due to a configuration change in activity, we
    // do nothing. Otherwise, we make this service a foreground service.
    removeLocationUpdates();
    if (!mChangingConfiguration)
    {
      Logger.i(TAG, "Starting foreground service");
      startForeground(NOTIFICATION_ID, getNotification());
    }
    return true;
  }

  public void doForeground()
  {
    if(!serviceIsRunningInForeground(this))
    {
      Logger.i(TAG, "Starting foreground service");
      startForeground(NOTIFICATION_ID, getNotification());
    }
  }

  @NonNull
  private Notification getNotification()
  {
    Intent stopSelf = new Intent(this, NavigationService.class);
    stopSelf.putExtra(EXTRA_STOP_SERVICE, true);
    final int FLAG_IMMUTABLE = Build.VERSION.SDK_INT < Build.VERSION_CODES.M ? 0 : PendingIntent.FLAG_IMMUTABLE;
    PendingIntent pStopSelf = PendingIntent.getService(this, 0, stopSelf,
        PendingIntent.FLAG_CANCEL_CURRENT | FLAG_IMMUTABLE);

    // TODO (@velichkomarija): restore navigation from notification.
    PendingIntent activityPendingIntent = PendingIntent
        .getActivity(this, 0,
                     new Intent(this, MwmActivity.class), FLAG_IMMUTABLE);

    Builder builder = new Builder(this, CHANNEL_ID)
        .addAction(R.drawable.ic_cancel, getString(R.string.button_exit),
                   pStopSelf)
        .setContentIntent(activityPendingIntent)
        .setOngoing(true)
        .setStyle(new NotificationCompat.DecoratedCustomViewStyle())
        .setCustomContentView(mRemoteViews)
        .setCustomHeadsUpContentView(mRemoteViews)
        .setPriority(Notification.PRIORITY_HIGH)
        .setSmallIcon(R.drawable.ic_notification)
        .setShowWhen(true);

    if (Utils.isOreoOrLater())
      builder.setChannelId(CHANNEL_ID);

    return builder.build();
  }

  private void updateNotification(@Nullable RoutingInfo routingInfo)
  {
    final String[] turnNotifications = Framework.nativeGenerateNotifications();
    if (turnNotifications != null)
    {
      mNavigationText = StringUtils.fixCaseInString(turnNotifications[0]);
      TtsPlayer.INSTANCE.playTurnNotifications(getApplicationContext(), turnNotifications);
    }
    mRemoteViews.setTextViewText(R.id.navigation_text, mNavigationText);

    StringBuilder stringBuilderNavigationSecondaryText = new StringBuilder();
    final RoutingController routingController = RoutingController.get();
    String routingArriveString = getString(R.string.routing_arrive);
    stringBuilderNavigationSecondaryText
        .append(String.format(routingArriveString, routingController.getEndPoint().getName()));
    if (routingInfo != null)
    {
      stringBuilderNavigationSecondaryText
          .append(": ")
          .append(Utils.calculateFinishTime(routingInfo.totalTimeInSeconds));
      mRemoteViews.setImageViewResource(R.id.navigation_icon, routingInfo.carDirection.getTurnRes());
      mRemoteViews.setTextViewText(R.id.navigation_distance_text, routingInfo.distToTurn + " " + routingInfo.turnUnits);
    }
    mRemoteViews.setTextViewText(R.id.navigation_secondary_text, stringBuilderNavigationSecondaryText
        .toString());
  }

  private void removeLocationUpdates()
  {
    Logger.i(TAG, "Removing location updates");
    LocationHelper.INSTANCE.removeListener(mLocationListener);
    stopSelf();
  }

  private boolean serviceIsRunningInForeground(@NonNull Context context)
  {
    ActivityManager manager = (ActivityManager) context.getSystemService(
        Context.ACTIVITY_SERVICE);
    // TODO(@velichkomarija): replace getRunningServices().
    // See issue https://github.com/android/location-samples/pull/243
    for (ActivityManager.RunningServiceInfo service : manager.getRunningServices(Integer.MAX_VALUE))
    {
      if (getClass().getName().equals(service.service.getClassName()))
      {
        if (service.foreground)
          return true;
      }
    }
    return false;
  }
}
