package com.mapswithme.maps;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.mapswithme.maps.background.NotificationCandidate;
import com.mapswithme.maps.geofence.GeoFenceFeature;
import com.mapswithme.maps.geofence.GeofenceLocation;

import java.util.Arrays;
import java.util.Collections;
import java.util.List;

public class LightFramework
{
  public static native boolean nativeIsAuthenticated();
  public static native int nativeGetNumberUnsentUGC();
  @NonNull
  private static native GeoFenceFeature[] nativeGetLocalAdsFeatures(double lat, double lon,
                                                                   double radiusInMeters,
                                                                   int maxCount);
  @NonNull
  public static List<GeoFenceFeature> getLocalAdsFeatures(double lat, double lon,
                                                          double radiusInMeters,
                                                          int maxCount)
  {
    return Collections.unmodifiableList(Arrays.asList(nativeGetLocalAdsFeatures(lat,
                                                                                lon,
                                                                                radiusInMeters,
                                                                                maxCount)));
  }

  public static void logLocalAdsEvent(@NonNull GeofenceLocation location,
                                      @NonNull GeoFenceFeature feature)
  {
    nativeLogLocalAdsEvent(1, location.getLat(), location.getLon(),
                           /* FIXME */
                           (int) location.getRadiusInMeters(), feature.getMwmVersion(),
                           feature.getCountryId(), feature.getFeatureIndex());
  }

  private static native void nativeLogLocalAdsEvent(int type, double lat, double lon,
                                                    int accuracyInMeters, long mwmVersion,
                                                    @NonNull String countryId, int featureIndex);
  @Nullable
  public static native NotificationCandidate nativeGetNotification();
}
