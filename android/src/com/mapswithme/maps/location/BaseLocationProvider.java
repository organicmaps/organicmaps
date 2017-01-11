package com.mapswithme.maps.location;

import android.location.Location;
import android.support.annotation.Nullable;

import com.mapswithme.util.LocationUtils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.SimpleLogger;

abstract class BaseLocationProvider
{
  static final Logger sLogger = SimpleLogger.get(BaseLocationProvider.class.getName());
  private static final double DEFAULT_SPEED_MPS = 5;

  boolean isLocationBetterThanLast(@Nullable Location newLocation)
  {
    if (newLocation == null)
      return false;

    final Location lastLocation = LocationHelper.INSTANCE.getSavedLocation();
    return (lastLocation == null || isLocationBetterThanLast(newLocation, lastLocation));
  }

  boolean isLocationBetterThanLast(Location newLocation, Location lastLocation)
  {
    double speed = Math.max(DEFAULT_SPEED_MPS, (newLocation.getSpeed() + lastLocation.getSpeed()) / 2.0);
    double lastAccuracy = (lastLocation.getAccuracy() + speed * LocationUtils.getDiff(lastLocation, newLocation));
    return (newLocation.getAccuracy() < lastAccuracy);
  }

  /**
   * @return whether location polling was started successfully.
   */
  protected abstract boolean start();
  protected abstract void stop();
}
