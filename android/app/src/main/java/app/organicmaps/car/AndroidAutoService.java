package app.organicmaps.car;

import android.content.ComponentName;
import android.content.Intent;
import android.net.Uri;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.car.app.CarContext;
import androidx.car.app.Session;
import androidx.car.app.SessionInfo;
import androidx.car.app.notification.CarAppExtender;
import androidx.car.app.notification.CarPendingIntent;
import androidx.core.app.NotificationChannelCompat;
import androidx.core.app.NotificationCompat;
import androidx.core.app.NotificationManagerCompat;
import app.organicmaps.BuildConfig;
import app.organicmaps.MwmApplication;
import app.organicmaps.R;
import app.organicmaps.api.Const;
import app.organicmaps.sdk.OrganicMaps;
import app.organicmaps.sdk.util.Config;
import app.organicmaps.sdk.util.log.Logger;
import java.io.IOException;

public final class AndroidAutoService extends CarAppServiceBase
{
  @NonNull
  private static final String TAG = AndroidAutoService.class.getSimpleName();

  @NonNull
  public static final String ANDROID_AUTO_NOTIFICATION_CHANNEL_ID = "ANDROID_AUTO";

  @Nullable
  private static NotificationCompat.Extender mCarNotificationExtender;

  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private OrganicMaps mOrganicMapsContext;
  private boolean mInitFailed = false;

  public AndroidAutoService()
  {
    super(/* isDebug */ BuildConfig.DEBUG);
  }

  @NonNull
  @Override
  public Session onCreateSession(@Nullable SessionInfo sessionInfo)
  {
    createNotificationChannel();
    return new AndroidAutoSession(mOrganicMapsContext, sessionInfo, mInitFailed);
  }

  @NonNull
  @Override
  public Session onCreateSession()
  {
    return onCreateSession(null);
  }

  @Override
  public void onCreate()
  {
    final MwmApplication app = MwmApplication.from(getApplicationContext());
    mOrganicMapsContext = app.getOrganicMaps();
    if (!mOrganicMapsContext.arePlatformAndCoreInitialized())
    {
      try
      {
        app.initOrganicMaps(null);
      }
      catch (IOException e)
      {
        Logger.e(TAG, "Failed to initialize the app: " + e.getMessage());
        mInitFailed = true;
      }
    }

    // TODO: Show dialog to the user
    Config.setFirstStartDialogSeen(getApplicationContext());
  }

  @NonNull
  public static NotificationCompat.Extender getCarNotificationExtender(@NonNull CarContext context)
  {
    if (mCarNotificationExtender != null)
      return mCarNotificationExtender;

    final Intent intent = new Intent(Intent.ACTION_VIEW)
                              .setComponent(new ComponentName(context, AndroidAutoService.class))
                              .setData(Uri.fromParts(Const.API_SCHEME, CarAppServiceBase.API_CAR_HOST,
                                                     CarAppServiceBase.ACTION_SHOW_NAVIGATION_SCREEN));
    mCarNotificationExtender = new CarAppExtender.Builder()
                                   .setImportance(NotificationManagerCompat.IMPORTANCE_MIN)
                                   .setContentIntent(CarPendingIntent.getCarApp(context, intent.hashCode(), intent, 0))
                                   .build();

    return mCarNotificationExtender;
  }

  private void createNotificationChannel()
  {
    final NotificationManagerCompat notificationManager = NotificationManagerCompat.from(this);
    final NotificationChannelCompat notificationChannel =
        new NotificationChannelCompat
            .Builder(ANDROID_AUTO_NOTIFICATION_CHANNEL_ID, NotificationManagerCompat.IMPORTANCE_MIN)
            .setName(getString(R.string.car_notification_channel_name))
            .setLightsEnabled(false) // less annoying
            .setVibrationEnabled(false) // less annoying
            .build();
    notificationManager.createNotificationChannel(notificationChannel);
  }
}
