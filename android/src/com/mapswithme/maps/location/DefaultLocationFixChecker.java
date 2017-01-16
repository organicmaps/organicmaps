package com.mapswithme.maps.location;

import android.location.Location;
import android.support.annotation.Nullable;

import com.mapswithme.util.LocationUtils;

class DefaultLocationFixChecker implements LocationFixChecker
{
  private static final double DEFAULT_SPEED_MPS = 5;

  @Override
  public boolean isLocationBetterThanLast(@Nullable Location newLocation)
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
}
