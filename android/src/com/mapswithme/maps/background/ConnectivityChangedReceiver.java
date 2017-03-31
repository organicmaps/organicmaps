package com.mapswithme.maps.background;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.downloader.MapManager;
import com.mapswithme.util.ConnectionState;

import static com.mapswithme.maps.MwmApplication.prefs;

public class ConnectivityChangedReceiver extends BroadcastReceiver
{
  private static final String DOWNLOAD_UPDATE_TIMESTAMP = "DownloadOrUpdateTimestamp";
  private static final long MIN_EVENT_DELTA_MILLIS = 3 * 60 * 60 * 1000; // 3 hours

  @Override
  public void onReceive(Context context, Intent intent)
  {
    MwmApplication.get().initNativePlatform();
    if (!ConnectionState.isWifiConnected()
        || MapManager.nativeNeedMigrate())
      return;

    final long lastEventTimestamp = prefs().getLong(DOWNLOAD_UPDATE_TIMESTAMP, 0);

    if (System.currentTimeMillis() - lastEventTimestamp > MIN_EVENT_DELTA_MILLIS)
    {
      prefs().edit()
             .putLong(DOWNLOAD_UPDATE_TIMESTAMP, System.currentTimeMillis())
             .apply();

      MwmApplication.get().initNativeCore();

      MapManager.checkUpdates();
      WorkerService.startActionCheckLocation(context);
    }
  }
}
