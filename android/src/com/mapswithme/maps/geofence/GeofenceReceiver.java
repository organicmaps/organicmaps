package com.mapswithme.maps.geofence;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

public class GeofenceReceiver extends BroadcastReceiver
{
  @Override
  public void onReceive(Context context, Intent intent)
  {
    GeofenceTransitionsIntentService.enqueueWork(context, intent);
  }
}
