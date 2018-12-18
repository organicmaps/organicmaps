package com.mapswithme.maps;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

import com.mapswithme.maps.scheduling.GeofenceTransitionsIntentService;

public class GeofenceReceiver extends BroadcastReceiver
{
  @Override
  public void onReceive(Context context, Intent intent)
  {
    GeofenceTransitionsIntentService.enqueueWork(context, intent);
  }
}
