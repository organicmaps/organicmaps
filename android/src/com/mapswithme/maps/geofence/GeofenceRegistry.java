package com.mapswithme.maps.geofence;

import android.support.annotation.NonNull;

import com.mapswithme.maps.location.LocationPermissionNotGrantedException;

public interface GeofenceRegistry
{
  void registerGeofences(@NonNull GeofenceLocation location) throws LocationPermissionNotGrantedException;
  void unregisterGeofences() throws LocationPermissionNotGrantedException;
}
