package com.mapswithme.maps.location;

public class GeofenceLocation
{
  private final double mLat;
  private final double mLon;
  private final int mRadiusInMeters;

  public GeofenceLocation(double lat, double lon, int radiusInMeters)
  {
    mLat = lat;
    mLon = lon;
    mRadiusInMeters = radiusInMeters;
  }

  @Override
  public String toString()
  {
    final StringBuilder sb = new StringBuilder("GeofenceLocation{");
    sb.append("mLat=").append(getLat());
    sb.append(", mLon=").append(getLon());
    sb.append(", mRadiusInMeters=").append(getRadiusInMeters());
    sb.append('}');
    return sb.toString();
  }

  public double getLat()
  {
    return mLat;
  }

  public double getLon()
  {
    return mLon;
  }

  public int getRadiusInMeters()
  {
    return mRadiusInMeters;
  }
}
