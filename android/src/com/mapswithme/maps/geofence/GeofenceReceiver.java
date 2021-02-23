package com.mapswithme.maps.geofence;

import android.content.Context;
import android.content.Intent;

import com.mapswithme.maps.MwmBroadcastReceiver;

public class GeofenceReceiver extends MwmBroadcastReceiver
{
  @Override
  public void onReceiveInitialized(Context context, Intent intent)
  {
    GeofenceTransitionsIntentService.enqueueWork(context, intent);
  }
}
