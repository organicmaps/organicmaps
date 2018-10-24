package com.mapswithme.maps.geofence;

import android.support.annotation.NonNull;

/**
 * Represents CampaignFeature from core.
 */
public class GeoFenceFeature
{
  private final long mwmVersion;
  @NonNull
  private final String countryId;
  private final int featureIndex;
  private final double latitude;
  private final double longitude;

  public GeoFenceFeature(long mwmVersion, @NonNull String countryId, int featureIndex,
                         double latitude, double longitude)
  {
    this.mwmVersion = mwmVersion;
    this.countryId = countryId;
    this.featureIndex = featureIndex;
    this.latitude = latitude;
    this.longitude = longitude;
  }

  public long getMwmVersion()
  {
    return mwmVersion;
  }

  @NonNull
  public String getCountryId()
  {
    return countryId;
  }

  public int getFeatureIndex()
  {
    return featureIndex;
  }

  public double getLatitude()
  {
    return latitude;
  }

  public double getLongitude()
  {
    return longitude;
  }
}
