package com.mapswithme.maps.background;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.downloader.MapManager;
import com.mapswithme.util.ConnectionState;
import com.mapswithme.util.CrashlyticsUtils;
import com.mapswithme.util.PermissionsUtils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

import static com.mapswithme.maps.MwmApplication.backgroundTracker;
import static com.mapswithme.maps.MwmApplication.prefs;

public class ConnectivityChangedReceiver extends BroadcastReceiver
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
  private static final String TAG = ConnectivityChangedReceiver.class.getSimpleName();
  private static final String DOWNLOAD_UPDATE_TIMESTAMP = "DownloadOrUpdateTimestamp";
  private static final long MIN_EVENT_DELTA_MILLIS = 3 * 60 * 60 * 1000; // 3 hours

  @Override
  public void onReceive(Context context, Intent intent)
  {
    String msg = "onReceive: " + intent + " app in background = "
                 + !backgroundTracker().isForeground();
    LOGGER.i(TAG, msg);
    CrashlyticsUtils.log(Log.INFO, TAG, msg);
    if (!PermissionsUtils.isExternalStorageGranted())
      return;

    MwmApplication application = MwmApplication.get();
    if (!application.arePlatformAndCoreInitialized())
    {
      boolean success = application.initCore();
      if (!success)
      {
        String message = "Native part couldn't be initialized successfully";
        LOGGER.e(TAG, message);
        CrashlyticsUtils.log(Log.ERROR, TAG, message);
        return;
      }
    }

    if (!ConnectionState.isWifiConnected()
        || MapManager.nativeNeedMigrate())
      return;

    final long lastEventTimestamp = prefs().getLong(DOWNLOAD_UPDATE_TIMESTAMP, 0);

    if (System.currentTimeMillis() - lastEventTimestamp > MIN_EVENT_DELTA_MILLIS)
    {
      prefs().edit()
             .putLong(DOWNLOAD_UPDATE_TIMESTAMP, System.currentTimeMillis())
             .apply();

      MapManager.checkUpdates();
      WorkerService.startActionCheckLocation(context);
    }
  }
}
