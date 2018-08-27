package com.mapswithme.maps.background;

import android.content.Context;
import android.content.Intent;
import android.net.ConnectivityManager;
import android.support.annotation.NonNull;
import android.support.v4.app.JobIntentService;

import com.mapswithme.maps.LightFramework;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.routing.RoutingController;
import com.mapswithme.util.NetworkPolicy;
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
  private static final int MIN_COUNT_UNSENT_UGC = 2;
  private static final long MIN_AUTH_EVENT_DELTA_MILLIS = TimeUnit.DAYS.toMillis(5);

  private interface NotificationExecutor
  {
    boolean tryToNotify();
  }

  public static void startOnConnectivityChanged(Context context)
  {
    final Intent intent = new Intent(context, NotificationService.class)
        .setAction(CONNECTIVITY_ACTION);

    final int jobId = NotificationService.class.hashCode();
    JobIntentService.enqueueWork(context, NotificationService.class, jobId, intent);
  }

  private static boolean notifyIsNotAuthenticated()
  {
    if (!PermissionsUtils.isExternalStorageGranted() ||
        !NetworkPolicy.getCurrentNetworkUsageStatus() ||
        LightFramework.nativeIsAuthenticated() ||
        LightFramework.nativeGetNumberUnsentUGC() < MIN_COUNT_UNSENT_UGC)
    {
      LOGGER.d(TAG, "Authentication notification is rejected. External storage granted: " +
                    PermissionsUtils.isExternalStorageGranted() + ". Is user authenticated: " +
                    LightFramework.nativeIsAuthenticated() + ". Current network usage status: " +
                    NetworkPolicy.getCurrentNetworkUsageStatus() + ". Number of unsent UGC: " +
                    LightFramework.nativeGetNumberUnsentUGC());
      return false;
    }

    // Do not show push when user is in the navigation mode.
    if (MwmApplication.get().arePlatformAndCoreInitialized() &&
        RoutingController.get().isNavigating())
    {
      LOGGER.d(TAG, "Authentication notification is rejected. The user is in navigation mode.");
      return false;
    }

    final long lastEventTimestamp = prefs().getLong(LAST_AUTH_NOTIFICATION_TIMESTAMP, 0);

    if (System.currentTimeMillis() - lastEventTimestamp > MIN_AUTH_EVENT_DELTA_MILLIS)
    {
      LOGGER.d(TAG, "Authentication notification will be sent.");

      prefs().edit()
             .putLong(LAST_AUTH_NOTIFICATION_TIMESTAMP, System.currentTimeMillis())
             .apply();

      Notifier.notifyAuthentication();

      return true;
    }
    LOGGER.d(TAG, "Authentication notification is rejected. Last event timestamp: " +
                  lastEventTimestamp + "Current time milliseconds: " + System.currentTimeMillis());
    return false;
  }

  @Override
  protected void onHandleWork(@NonNull Intent intent)
  {
    final String action = intent.getAction();

    if (ConnectivityManager.CONNECTIVITY_ACTION.equals(action))
      onConnectivityChanged();
  }

  private static void onConnectivityChanged()
  {
    final NotificationExecutor notifyOrder[] =
    {
        NotificationService::notifyIsNotAuthenticated,
    };

    // Only one notification should be shown at a time.
    for (NotificationExecutor executor : notifyOrder)
    {
      if (executor.tryToNotify())
        return;
    }
  }
}
