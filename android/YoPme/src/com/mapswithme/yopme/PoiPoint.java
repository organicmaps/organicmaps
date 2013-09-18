package com.mapswithme.yopme;

import java.io.Serializable;

public class PoiPoint implements Serializable
{
  private static final long serialVersionUID = 1L;
  private final double mLon;
  private final double mLat;
  private final String mName;

  public PoiPoint(double lat, double lon, String name)
  {
    mLat = lat;
    mLon = lon;
    mName = name;
  }

  public String getName() { return mName; }
  public double getLat()  { return mLat; }
  public double getLon()  { return mLon; }
}