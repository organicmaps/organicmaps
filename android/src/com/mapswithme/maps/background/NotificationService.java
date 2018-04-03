package com.mapswithme.maps.background;

import android.app.IntentService;
import android.content.Context;
import android.content.Intent;
import android.support.annotation.Nullable;

import com.mapswithme.maps.LightFramework;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.routing.RoutingController;
import com.mapswithme.util.NetworkPolicy;
import com.mapswithme.util.PermissionsUtils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

import static com.mapswithme.maps.MwmApplication.prefs;

public class NotificationService extends IntentService
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
  private static final String TAG = NotificationService.class.getSimpleName();
  private static final String LAST_AUTH_NOTIFICATION_TIMESTAMP = "DownloadOrUpdateTimestamp";
  private static final int MIN_COUNT_UNSENT_UGC = 2;
  private static final long MIN_AUTH_EVENT_DELTA_MILLIS = 5 * 24 * 60 * 60 * 1000; // 5 days
  private static final String CONNECTIVITY_CHANGED =
      "com.mapswithme.maps.notification_service.action.connectivity_changed";

  private interface NotificationExecutor
  {
    boolean tryToNotify();
  }

  static void startOnConnectivityChanged(Context context)
  {
    final Intent intent = new Intent(context, NotificationService.class);
    intent.setAction(NotificationService.CONNECTIVITY_CHANGED);
    context.startService(intent);
  }

  private static boolean notifyIsNotAuthenticated()
  {
    if (!PermissionsUtils.isExternalStorageGranted() ||
        !NetworkPolicy.getCurrentNetworkUsageStatus() ||
        LightFramework.nativeIsAuthenticated() ||
        LightFramework.nativeGetNumberUnsentUGC() < MIN_COUNT_UNSENT_UGC)
    {
      LOGGER.d(TAG, "Authentication nofification is rejected. External storage granted: " +
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
      LOGGER.d(TAG, "Authentication nofification is rejected. The user is in navigation mode.");
      return false;
    }

    final long lastEventTimestamp = prefs().getLong(LAST_AUTH_NOTIFICATION_TIMESTAMP, 0);

    if (System.currentTimeMillis() - lastEventTimestamp > MIN_AUTH_EVENT_DELTA_MILLIS)
    {
      LOGGER.d(TAG, "Authentication nofification will be sent.");

      prefs().edit()
             .putLong(LAST_AUTH_NOTIFICATION_TIMESTAMP, System.currentTimeMillis())
             .apply();

      Notifier.notifyIsNotAuthenticated();

      return true;
    }
    LOGGER.d(TAG, "Authentication nofification is rejected. Last event timestamp: " +
                  lastEventTimestamp + "Current time milliseconds: " + System.currentTimeMillis());
    return false;
  }

  public NotificationService()
  {
    super(NotificationService.class.getSimpleName());
  }

  @Override
  protected void onHandleIntent(@Nullable Intent intent)
  {
    if (intent == null)
      return;

    final String action = intent.getAction();

    if (action == null)
      return;

    switch(action)
    {
      case CONNECTIVITY_CHANGED:
        onConnectivityChanged();
        break;
    }
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
