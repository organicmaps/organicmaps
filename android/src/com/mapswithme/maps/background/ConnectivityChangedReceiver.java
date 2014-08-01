package com.mapswithme.maps.background;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;

public class ConnectivityChangedReceiver extends BroadcastReceiver
{
  @Override
  public void onReceive(Context context, Intent intent)
  {
    final NetworkInfo networkInfo = intent.getParcelableExtra(ConnectivityManager.EXTRA_NETWORK_INFO);
    if (networkInfo != null)
      if (networkInfo.getType() == ConnectivityManager.TYPE_WIFI)
        onWiFiConnectionChanged(networkInfo.isConnected(), context);
  }

  public void onWiFiConnectionChanged(boolean isConnected, Context context)
  {
    if (isConnected)
    {
      WorkerService.startActionDownload(context);
      WorkerService.startActionCheckUpdate(context);
      WorkerService.startActionPushStat(context);
    }
  }
}
