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

  @Override
  public boolean equals(Object o)
  {
    if (this == o) return true;
    if (o == null || getClass() != o.getClass()) return false;

    GeoFenceFeature that = (GeoFenceFeature) o;

    if (mwmVersion != that.mwmVersion) return false;
    if (featureIndex != that.featureIndex) return false;
    return countryId.equals(that.countryId);
  }

  @Override
  public int hashCode()
  {
    int result = (int) (mwmVersion ^ (mwmVersion >>> 32));
    result = 31 * result + countryId.hashCode();
    result = 31 * result + featureIndex;
    return result;
  }
}
