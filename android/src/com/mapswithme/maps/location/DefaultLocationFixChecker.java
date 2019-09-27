package com.mapswithme.maps.location;

import android.location.Location;
import androidx.annotation.NonNull;

import com.mapswithme.util.LocationUtils;

class DefaultLocationFixChecker implements LocationFixChecker
{
  private static final double DEFAULT_SPEED_MPS = 5;
  private static final String GPS_LOCATION_PROVIDER = "gps";

  @Override
  public boolean isAccuracySatisfied(@NonNull Location location)
  {
    // If it's a gps location then we completely ignore an accuracy checking,
    // because there are cases on some devices (https://jira.mail.ru/browse/MAPSME-3789)
    // when location is good, but it doesn't contain an accuracy for some reasons
    if (isFromGpsProvider(location))
      return true;

    // Completely ignore locations without lat and lon
    return location.getAccuracy() > 0.0f;
  }

  private static boolean isFromGpsProvider(@NonNull Location location)
  {
    return GPS_LOCATION_PROVIDER.equals(location.getProvider());
  }

  @Override
  public boolean isLocationBetterThanLast(@NonNull Location newLocation)
  {
    final Location lastLocation = LocationHelper.INSTANCE.getSavedLocation();

    if (lastLocation == null)
      return true;

    //noinspection SimplifiableIfStatement
    if (isFromGpsProvider(lastLocation) && lastLocation.getAccuracy() == 0.0f)
      return true;

    return isLocationBetterThanLast(newLocation, lastLocation);
  }

  boolean isLocationBetterThanLast(@NonNull Location newLocation, @NonNull Location lastLocation)
  {
    double speed = Math.max(DEFAULT_SPEED_MPS, (newLocation.getSpeed() + lastLocation.getSpeed()) / 2.0);
    double lastAccuracy = lastLocation.getAccuracy() + speed * LocationUtils.getDiff(lastLocation, newLocation);
    return newLocation.getAccuracy() < lastAccuracy;
  }
}
