package com.mapswithme.maps.location;

import android.support.annotation.NonNull;

public interface GeofenceRegistry
{
  void registryGeofences(@NonNull GeofenceLocation location);
  void invalidateGeofences();
}
