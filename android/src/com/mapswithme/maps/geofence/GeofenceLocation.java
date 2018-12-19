package com.mapswithme.maps.geofence;

import android.location.Location;
import android.support.annotation.NonNull;

public class GeofenceLocation
{
  private final double mLat;
  private final double mLon;
  private final float mRadiusInMeters;

  private GeofenceLocation(double lat, double lon, float radiusInMeters)
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

  public float getRadiusInMeters()
  {
    return mRadiusInMeters;
  }

  @NonNull
  public static GeofenceLocation from(@NonNull Location location)
  {
    return new GeofenceLocation(location.getLatitude(), location.getLongitude(), location.getAccuracy());
  }
}
