package com.mapswithme.maps.location;

import android.location.Location;
import android.support.annotation.Nullable;

interface LocationFixChecker
{
  boolean isLocationBetterThanLast(@Nullable Location newLocation);
}
