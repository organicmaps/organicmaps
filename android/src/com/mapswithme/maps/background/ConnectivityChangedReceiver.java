package com.mapswithme.maps.background;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.util.ConnectionState;

public class ConnectivityChangedReceiver extends BroadcastReceiver
{
  private static final String DOWNLOAD_UPDATE_TIMESTAMP = "DownloadOrUpdateTimestamp";
  private static final long MIN_EVENT_DELTA_MILLIS = 3 * 60 * 60 * 1000; // 3 hours

  @Override
  public void onReceive(Context context, Intent intent)
  {
    if (ConnectionState.isWifiConnected())
      onWiFiConnected(context);
  }

  public void onWiFiConnected(Context context)
  {
    final SharedPreferences prefs = MwmApplication.prefs();
    final long lastEventTimestamp = prefs.getLong(DOWNLOAD_UPDATE_TIMESTAMP, 0);

    if (System.currentTimeMillis() - lastEventTimestamp > MIN_EVENT_DELTA_MILLIS)
    {
      prefs.edit().putLong(DOWNLOAD_UPDATE_TIMESTAMP, System.currentTimeMillis()).apply();
      WorkerService.startActionDownload(context);
      WorkerService.startActionCheckUpdate(context);
    }
  }
}
