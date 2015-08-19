package com.mapswithme.maps.bookmarks.data;

public class DistanceAndAzimut
{
  private String mDistance;
  private double mAzimuth;

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
