package app.organicmaps.car;

import androidx.annotation.NonNull;
import androidx.car.app.Session;
import androidx.car.app.SessionInfo;
import androidx.core.app.NotificationChannelCompat;
import androidx.core.app.NotificationManagerCompat;
import app.organicmaps.BuildConfig;
import app.organicmaps.MwmApplication;
import app.organicmaps.R;
import app.organicmaps.sdk.OrganicMaps;
import app.organicmaps.sdk.car.CarType;
import app.organicmaps.sdk.car.CarTypeHelper;
import app.organicmaps.sdk.display.DisplayManager;
import app.organicmaps.sdk.display.DisplayType;
import app.organicmaps.sdk.util.Config;
import app.organicmaps.sdk.util.log.Logger;
import java.io.IOException;

public final class AndroidAutoService extends CarAppServiceBase
{
  @NonNull
  private static final String TAG = AndroidAutoService.class.getSimpleName();

  @NonNull
  public static final String ANDROID_AUTO_NOTIFICATION_CHANNEL_ID = "ANDROID_AUTO";

  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private OrganicMaps mOrganicMapsContext;
  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private DisplayManager mDisplayManager;
  private boolean mInitFailed = false;

  public AndroidAutoService()
  {
    super(/* isDebug */ BuildConfig.DEBUG);
  }

  @NonNull
  @Override
  public Session onCreateSession(@NonNull SessionInfo sessionInfo)
  {
    mDisplayManager.init(DisplayType.Car);
    return new AndroidAutoSession(mOrganicMapsContext, mDisplayManager, sessionInfo, /* isDebug */ BuildConfig.DEBUG,
                                  mInitFailed);
  }

  @Override
  public void onCreate()
  {
    super.onCreate();
    createNotificationChannel();
    CarTypeHelper.setCarType(CarType.AndroidAuto);

    final MwmApplication app = MwmApplication.from(getApplicationContext());
    mOrganicMapsContext = app.getOrganicMaps();
    mDisplayManager = app.getDisplayManager();

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
