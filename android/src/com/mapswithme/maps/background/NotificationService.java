package com.mapswithme.maps.background;

import android.content.Context;
import android.content.Intent;
import android.net.ConnectivityManager;
import androidx.annotation.NonNull;
import androidx.core.app.JobIntentService;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.routing.RoutingController;
import com.mapswithme.maps.scheduling.JobIdMap;
import com.mapswithme.util.PermissionsUtils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

import java.util.concurrent.TimeUnit;

import static android.net.ConnectivityManager.CONNECTIVITY_ACTION;
import static com.mapswithme.maps.MwmApplication.prefs;

public class NotificationService extends JobIntentService
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
  private static final String TAG = NotificationService.class.getSimpleName();
  private static final String LAST_AUTH_NOTIFICATION_TIMESTAMP = "DownloadOrUpdateTimestamp";

  private interface NotificationExecutor
  {
    boolean tryToNotify();
  }

  public static void startOnConnectivityChanged(@NonNull Context context)
  {
    final Intent intent = new Intent(context, NotificationService.class)
        .setAction(CONNECTIVITY_ACTION);

    int id = JobIdMap.getId(NotificationService.class.hashCode();
    JobIntentService.enqueueWork(context, NotificationService.class, id, intent);
  }

  private boolean notifyIsNotAuthenticated()
  {
    final long lastEventTimestamp = prefs(this)
        .getLong(LAST_AUTH_NOTIFICATION_TIMESTAMP, 0);

//    if (System.currentTimeMillis() - lastEventTimestamp > MIN_AUTH_EVENT_DELTA_MILLIS)
//    {
//      LOGGER.d(TAG, "Authentication notification will be sent.");
//
//      prefs(this).edit().putLong(LAST_AUTH_NOTIFICATION_TIMESTAMP, System.currentTimeMillis()).apply();
//
//      Notifier notifier = Notifier.from(getApplication());
//      notifier.notifyAuthentication();
//
//      return true;
//    }
    LOGGER.d(TAG, "Authentication notification is rejected. Last event timestamp: " +
                  lastEventTimestamp + "Current time milliseconds: " + System.currentTimeMillis());
    return false;
  }

  private boolean notifySmart()
  {
    return false;
  }

  @Override
  protected void onHandleWork(@NonNull Intent intent)
  {
    final String action = intent.getAction();

    if (ConnectivityManager.CONNECTIVITY_ACTION.equals(action))
      tryToShowNotification();
  }

  private void tryToShowNotification()
  {
    if (!PermissionsUtils.isLocationGranted(this))
    {
      LOGGER.d(TAG, "Notification is rejected. Location permission is not granted.");
      return;
    }

    // Do not show push when user is in the navigation mode.
    if (MwmApplication.from(this).arePlatformAndCoreInitialized()
        && RoutingController.get().isNavigating())
    {
      LOGGER.d(TAG, "Notification is rejected. The user is in navigation mode.");
      return;
    }

    final NotificationExecutor[] notifyOrder =
    {
      this::notifyIsNotAuthenticated,
      this::notifySmart
    };

    // Only one notification should be shown at a time.
    for (NotificationExecutor executor : notifyOrder)
    {
      if (executor.tryToNotify())
        return;
    }
  }
}
