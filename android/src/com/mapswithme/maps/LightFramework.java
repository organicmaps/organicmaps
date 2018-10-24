package com.mapswithme.maps;

import android.support.annotation.NonNull;

import com.mapswithme.maps.geofence.GeoFenceFeature;

public class LightFramework
{
  public static native boolean nativeIsAuthenticated();
  public static native int nativeGetNumberUnsentUGC();
  @NonNull
  public static native GeoFenceFeature[] nativeGetLocalAdsFeatures(double lat, double lon,
                                                                   double radiusInMeters,
                                                                   int maxCount);
  public static native void nativeLogLocalAdsEvent(int type, double lat, double lon,
                                                   int accuracyInMeters, long mwmVersion,
                                                   @NonNull String countryId, int featureIndex);
}
