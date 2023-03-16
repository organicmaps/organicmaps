package app.organicmaps.bookmarks.data;

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
