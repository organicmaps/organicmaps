package com.mapswithme.maps.background;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;

import com.mapswithme.maps.MWMApplication;

public class ConnectivityChangedReceiver extends BroadcastReceiver
{
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
    WorkerService.startActionDownload(context);
    WorkerService.startActionCheckUpdate(context);
    WorkerService.startActionPushStat(context);
  }
}
