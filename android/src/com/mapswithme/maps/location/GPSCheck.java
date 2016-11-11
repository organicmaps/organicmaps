package com.mapswithme.maps.location;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.location.LocationManager;
import android.util.Log;

import com.mapswithme.maps.MwmApplication;

public class GPSCheck extends BroadcastReceiver
{
  @Override
  public void onReceive(Context context, Intent intent) {

    LocationManager locationManager = (LocationManager) context.getSystemService(context.LOCATION_SERVICE);

    if ((locationManager.isProviderEnabled(LocationManager.GPS_PROVIDER)
        || locationManager.isProviderEnabled(LocationManager.NETWORK_PROVIDER))
        && MwmApplication.get().isFrameworkInitialized() && MwmApplication.backgroundTracker().isForeground())
    {
      LocationHelper.INSTANCE.addLocationListener();
      LocationHelper.INSTANCE.forceRestart();
    }
  }
}