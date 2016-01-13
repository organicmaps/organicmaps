package com.mapswithme.maps.location;


import android.location.Location;
import android.location.LocationListener;
import android.os.Bundle;

import com.mapswithme.util.LocationUtils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.SimpleLogger;

abstract class BaseLocationProvider implements LocationListener
{
  private static final double DEFAULT_SPEED_MPS = 5;

  protected static final Logger sLogger = SimpleLogger.get(BaseLocationProvider.class.getName());
  protected static final long UPDATE_INTERVAL_MS = 500;

  protected abstract void startUpdates();

  protected abstract void stopUpdates();

  protected boolean isLocationBetterThanLast(Location newLocation, Location lastLocation)
  {
    double speed = Math.max(DEFAULT_SPEED_MPS, (newLocation.getSpeed() + lastLocation.getSpeed()) / 2.0);
    double lastAccuracy = (lastLocation.getAccuracy() + speed * LocationUtils.getDiffWithLastLocation(lastLocation, newLocation));
    return (newLocation.getAccuracy() < lastAccuracy);
  }

  protected final boolean isLocationBetterThanLast(Location newLocation)
  {
    if (newLocation == null)
      return false;

    final Location lastLocation = LocationHelper.INSTANCE.getLastLocation();
    return (lastLocation == null || isLocationBetterThanLast(newLocation, lastLocation));
  }

  @Override
  public void onLocationChanged(Location location)
  {
    // Completely ignore locations without lat and lon
    if (location.getAccuracy() <= 0.0)
      return;

    if (isLocationBetterThanLast(location))
    {
      LocationHelper.INSTANCE.initMagneticField(location);
      LocationHelper.INSTANCE.setLastLocation(location);
    }
  }

  @Override
  public void onProviderDisabled(String provider)
  {
    sLogger.d("Disabled location provider: ", provider);
  }

  @Override
  public void onProviderEnabled(String provider)
  {
    sLogger.d("Enabled location provider: ", provider);
  }

  @Override
  public void onStatusChanged(String provider, int status, Bundle extras)
  {
    sLogger.d("Status changed for location provider: ", provider, status);
  }
}
