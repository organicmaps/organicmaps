package com.mapswithme.maps.api;

/**
 * Represents CampaignFeature from core.
 */
public class GeoFenceFeature
{
  public final long mwmVersion;
  public final String countryId;
  public final int featureIndex;
  public final double latitude;
  public final double longitude;

  public GeoFenceFeature(long mwmVersion, String countryId, int featureIndex,
                         double latitude, double longitude)
  {
    this.mwmVersion = mwmVersion;
    this.countryId = countryId;
    this.featureIndex = featureIndex;
    this.latitude = latitude;
    this.longitude = longitude;
  }
}
