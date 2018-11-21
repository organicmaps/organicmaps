package com.mapswithme.maps;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.mapswithme.maps.background.NotificationCandidate;
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
  @Nullable
  public static native NotificationCandidate nativeGetNotification();
}
