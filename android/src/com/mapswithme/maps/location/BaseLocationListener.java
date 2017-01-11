package com.mapswithme.maps.location;

import android.location.Location;
import android.location.LocationListener;
import android.os.Bundle;

import com.mapswithme.util.log.DebugLogger;
import com.mapswithme.util.log.Logger;

class BaseLocationListener implements LocationListener, com.google.android.gms.location.LocationListener
{
  private static final Logger LOGGER = new DebugLogger(BaseLocationListener.class.getSimpleName());

  @Override
  public void onLocationChanged(Location location)
  {
    // Completely ignore locations without lat and lon
    if (location.getAccuracy() <= 0.0)
      return;

    LocationHelper.INSTANCE.resetMagneticField(location);
    LocationHelper.INSTANCE.onLocationUpdated(location);
    LocationHelper.INSTANCE.notifyLocationUpdated();
  }

  @Override
  public void onProviderDisabled(String provider)
  {
    LOGGER.d("Disabled location provider: ", provider);
  }

  @Override
  public void onProviderEnabled(String provider)
  {
    LOGGER.d("Enabled location provider: ", provider);
  }

  @Override
  public void onStatusChanged(String provider, int status, Bundle extras)
  {
    LOGGER.d("Status changed for location provider: " + provider + "; new status = " + status);
  }
}
