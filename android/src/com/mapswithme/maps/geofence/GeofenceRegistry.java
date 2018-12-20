package com.mapswithme.maps.geofence;

import android.support.annotation.NonNull;

import com.google.android.gms.location.Geofence;
import com.mapswithme.maps.location.LocationPermissionNotGrantedException;

public interface GeofenceRegistry
{
  void registerGeofences(@NonNull GeofenceLocation location) throws LocationPermissionNotGrantedException;
  void unregisterGeofences() throws LocationPermissionNotGrantedException;

  @NonNull
  GeoFenceFeature getFeatureByGeofence(@NonNull Geofence geofence);
}
