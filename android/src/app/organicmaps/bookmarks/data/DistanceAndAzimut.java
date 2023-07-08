package app.organicmaps.bookmarks.data;

import app.organicmaps.util.Distance;

public class DistanceAndAzimut
{
  private final Distance mDistance;
  private final double mAzimuth;

  public Distance getDistance()
  {
    return mDistance;
  }

  public double getAzimuth()
  {
    return mAzimuth;
  }

  public DistanceAndAzimut(Distance distance, double azimuth)
  {
    mDistance = distance;
    mAzimuth = azimuth;
  }
}
