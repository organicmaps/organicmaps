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
    if (!ConnectivityManager.CONNECTIVITY_ACTION.equals(intent.getAction()))
      throw new IllegalStateException("This class must listen only to CONNECTIVITY_ACTION");

    @SuppressWarnings("deprecation")
    final NetworkInfo networkInfo = (NetworkInfo) intent.getParcelableExtra(ConnectivityManager.EXTRA_NETWORK_INFO);
    if (networkInfo != null)
    {
      if (networkInfo.getType() == ConnectivityManager.TYPE_WIFI)
        onWiFiConnectionChanged(networkInfo.isConnected(), context);
    }
  }

  public void onWiFiConnectionChanged(boolean isConnected, Context context)
  {
    if (isConnected)
    {
      WorkerService.startActionCheckUpdate(context);
      WorkerService.startActionPushStat(context);
    }
  }
}
