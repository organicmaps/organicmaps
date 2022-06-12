package com.mapswithme.maps.api;

import androidx.annotation.Keep;

/**
 * Represents url_scheme::RoutePoint from core.
 */
@Keep
public class RoutePoint
{
  public final double mLat;
  public final double mLon;
  public final String mName;

  public RoutePoint(double lat, double lon, String name)
  {
    mLat = lat;
    mLon = lon;
    mName = name;
  }
}
