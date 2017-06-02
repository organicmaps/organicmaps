package com.mapswithme.maps.background;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.BatteryManager;

import com.mapswithme.util.statistics.Statistics;

public class BatteryStatusReceiver extends BroadcastReceiver
{

  @Override
  public void onReceive(Context context, Intent intent)
  {
    int level = intent.getIntExtra(BatteryManager.EXTRA_LEVEL, 0);
    int chargePlug = intent.getIntExtra(BatteryManager.EXTRA_PLUGGED, -1);
    final String charging;
    if (chargePlug > 0)
      charging = "on";
    else if (chargePlug < 0)
      charging = "unknown";
    else
      charging = "off";

    Statistics.INSTANCE.trackColdStartupInfo(level, charging);
    context.unregisterReceiver(this);
  }
}
