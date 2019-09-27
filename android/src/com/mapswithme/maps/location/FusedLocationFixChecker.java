package com.mapswithme.maps.location;

import android.location.Location;
import androidx.annotation.NonNull;

class FusedLocationFixChecker extends DefaultLocationFixChecker
{
  private static final String GMS_LOCATION_PROVIDER = "fused";

  @Override
  boolean isLocationBetterThanLast(@NonNull Location newLocation, @NonNull Location lastLocation)
  {
    // We believe that google services always return good locations.
    return isFromFusedProvider(newLocation) ||
            (!isFromFusedProvider(lastLocation) && super.isLocationBetterThanLast(newLocation, lastLocation));
  }

  private static boolean isFromFusedProvider(Location location)
  {
    return GMS_LOCATION_PROVIDER.equalsIgnoreCase(location.getProvider());
  }
}
