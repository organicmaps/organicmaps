package com.mapswithme.maps.location;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.location.LocationManager;
import android.util.Log;

public class GPSCheck extends BroadcastReceiver
{
  @Override
  public void onReceive(Context context, Intent intent) {

    LocationManager locationManager = (LocationManager) context.getSystemService(context.LOCATION_SERVICE);

    if (locationManager.isProviderEnabled(LocationManager.GPS_PROVIDER))
    {
      LocationHelper.INSTANCE.addLocationListener();
    }
  }
}