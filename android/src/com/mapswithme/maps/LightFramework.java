package com.mapswithme.maps;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.mapswithme.maps.background.NotificationCandidate;
import com.mapswithme.maps.bookmarks.data.FeatureId;
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
  public static native String nativeMakeFeatureId(@NonNull String mwmName, long mwmVersion,
                                                  int featureIndex);

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
                                      @NonNull FeatureId feature)
  {
    nativeLogLocalAdsEvent(Framework.LocalAdsEventType.LOCAL_ADS_EVENT_VISIT.ordinal(),
                           location.getLat(), location.getLon(),
                           (int) location.getRadiusInMeters(), feature.getMwmVersion(),
                           feature.getMwmName(), feature.getFeatureIndex());
  }

  private static native void nativeLogLocalAdsEvent(int type, double lat, double lon,
                                                    int accuracyInMeters, long mwmVersion,
                                                    @NonNull String countryId, int featureIndex);
  @Nullable
  public static native NotificationCandidate nativeGetNotification();
}
