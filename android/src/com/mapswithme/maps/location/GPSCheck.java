package com.mapswithme.maps.location;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

import com.mapswithme.maps.MwmApplication;

public class GPSCheck extends BroadcastReceiver
{
  @Override
  public void onReceive(Context context, Intent intent) {
    if (MwmApplication.get().isFrameworkInitialized() && MwmApplication.backgroundTracker().isForeground())
    {
      LocationHelper.INSTANCE.restart();
    }
  }
}
