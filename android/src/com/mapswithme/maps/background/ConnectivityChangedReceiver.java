package com.mapswithme.maps.background;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;

import com.mapswithme.maps.MWMApplication;
import com.mapswithme.maps.R;

public class ConnectivityChangedReceiver extends BroadcastReceiver
{
  private static final String DOWNLOAD_UPDATE_TIMESTAMP = "DownloadOrUpdateTimestamp";
  private static final long MIN_EVENT_DELTA_MILLIS = 3 * 60 * 60 * 1000; // 3 hours

  @Override
  public void onReceive(Context context, Intent intent)
  {
    final ConnectivityManager manager = (ConnectivityManager) MWMApplication.get().getSystemService(Context.CONNECTIVITY_SERVICE);
    final NetworkInfo networkInfo = manager.getActiveNetworkInfo();
    if (networkInfo != null &&
        networkInfo.isConnected() &&
        networkInfo.getType() == ConnectivityManager.TYPE_WIFI)
      onWiFiConnected(context);
  }

  public void onWiFiConnected(Context context)
  {
    final SharedPreferences prefs = context.getSharedPreferences(context.getString(R.string.pref_file_name), Context.MODE_PRIVATE);
    final long lastEventTimestamp = prefs.getLong(DOWNLOAD_UPDATE_TIMESTAMP, 0);

    if (System.currentTimeMillis() - lastEventTimestamp > MIN_EVENT_DELTA_MILLIS)
    {
      prefs.edit().putLong(DOWNLOAD_UPDATE_TIMESTAMP, System.currentTimeMillis()).apply();
      WorkerService.startActionDownload(context);
      WorkerService.startActionCheckUpdate(context);
    }
  }
}
