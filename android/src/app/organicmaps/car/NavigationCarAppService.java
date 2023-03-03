package app.organicmaps.car;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.os.Build;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.car.app.CarAppService;
import androidx.car.app.Session;
import androidx.car.app.SessionInfo;
import androidx.car.app.validation.HostValidator;
import androidx.core.app.NotificationCompat;
import androidx.lifecycle.DefaultLifecycleObserver;
import androidx.lifecycle.LifecycleOwner;

import app.organicmaps.BuildConfig;
import app.organicmaps.R;

public final class NavigationCarAppService extends CarAppService
{
  /**
   * Navigation session channel id.
   */
  public static final String CHANNEL_ID = "NavigationSessionChannel";

  /**
   * The identifier for the notification displayed for the foreground service.
   */
  private static final int NOTIFICATION_ID = 97654321;

  @NonNull
  @Override
  public HostValidator createHostValidator()
  {
    if (BuildConfig.DEBUG)
      return HostValidator.ALLOW_ALL_HOSTS_VALIDATOR;

    return new HostValidator.Builder(getApplicationContext())
        .addAllowedHosts(androidx.car.app.R.array.hosts_allowlist_sample)
        .build();
  }

  @NonNull
  @Override
  public Session onCreateSession(@Nullable SessionInfo sessionInfo)
  {
    createNotificationChannel();

    // Turn the car app service into a foreground service in order to make sure we can use all
    // granted "while-in-use" permissions (e.g. location) in the app's process.
    // The "while-in-use" location permission is granted as long as there is a foreground
    // service running in a process in which location access takes place. Here, we set this
    // service, and not NavigationService (which runs only during navigation), as a
    // foreground service because we need location access even when not navigating. If
    // location access is needed only during navigation, we can set NavigationService as a
    // foreground service instead.
    // See https://developer.android.com/reference/com/google/android/libraries/car/app
    // /CarAppService#accessing-location for more details.
    startForeground(NOTIFICATION_ID, getNotification());
    final NavigationSession navigationSession = new NavigationSession(sessionInfo);
    navigationSession.getLifecycle().addObserver(new DefaultLifecycleObserver()
    {
      @Override
      public void onDestroy(@NonNull LifecycleOwner owner)
      {
        stopForeground(true);
      }
    });
    return navigationSession;
  }

  @NonNull
  @Override
  public Session onCreateSession()
  {
    return onCreateSession(null);
  }

  private void createNotificationChannel()
  {
    NotificationManager notificationManager = getSystemService(NotificationManager.class);
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O)
    {
      CharSequence name = "Car App Service";
      NotificationChannel serviceChannel =
          new NotificationChannel(CHANNEL_ID, name, NotificationManager.IMPORTANCE_HIGH);
      notificationManager.createNotificationChannel(serviceChannel);
    }
  }

  /**
   * Returns the {@link NotificationCompat} used as part of the foreground service.
   */
  private Notification getNotification()
  {
    NotificationCompat.Builder builder =
        new NotificationCompat.Builder(this, CHANNEL_ID)
            .setContentTitle("Navigation App")
            .setContentText("App is running")
            .setSmallIcon(R.mipmap.ic_launcher);

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O)
    {
      builder.setChannelId(CHANNEL_ID);
      builder.setPriority(NotificationManager.IMPORTANCE_HIGH);
    }
    return builder.build();
  }
}
