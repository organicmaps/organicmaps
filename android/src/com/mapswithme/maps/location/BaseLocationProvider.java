package com.mapswithme.maps.location;


import android.location.Location;

import com.mapswithme.util.LocationUtils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.SimpleLogger;

public abstract class BaseLocationProvider
{
  protected static final long LOCATION_UPDATE_INTERVAL = 500;
  public static final double DEFAULT_SPEED_MPS = 5;
  public static final float DISTANCE_TO_RECREATE_MAGNETIC_FIELD_M = 1000;

  protected static final Logger mLogger = SimpleLogger.get(BaseLocationProvider.class.getName());

  protected abstract void startUpdates();

  protected abstract void stopUpdates();

  protected boolean isLocationBetterThanLast(Location newLocation)
  {
    if (newLocation == null)
      return false;

    final Location lastLocation = LocationService.INSTANCE.getLastLocation();
    if (lastLocation == null)
      return true;

    final double s = Math.max(DEFAULT_SPEED_MPS, (newLocation.getSpeed() + lastLocation.getSpeed()) / 2.0);
    return (newLocation.getAccuracy() < (lastLocation.getAccuracy() + s * LocationUtils.getDiffWithLastLocation(lastLocation, newLocation)));
  }
}
