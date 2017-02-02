package com.mapswithme.maps.location;

import android.location.Location;
import android.support.annotation.NonNull;

interface LocationFixChecker
{
  boolean isLocationBetterThanLast(@NonNull Location newLocation);
  boolean isAccuracySatisfied(@NonNull Location location);
}
