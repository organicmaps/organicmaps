package com.mapswithme.maps.bookmarks.data;

import androidx.annotation.Keep;

@Keep
public class DistanceAndAzimut
{
  private final String mDistance;
  private final double mAzimuth;

  public String getDistance()
  {
    return mDistance;
  }

  public double getAzimuth()
  {
    return mAzimuth;
  }

  public DistanceAndAzimut(String distance, double azimuth)
  {
    mDistance = distance;
    mAzimuth = azimuth;
  }
}
