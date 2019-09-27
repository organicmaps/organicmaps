package com.mapswithme.maps.location;

import android.location.Location;
import androidx.annotation.NonNull;

interface LocationFixChecker
{
  boolean isLocationBetterThanLast(@NonNull Location newLocation);
  boolean isAccuracySatisfied(@NonNull Location location);
}
