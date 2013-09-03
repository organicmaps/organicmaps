package com.mapswithme.yopme.map;

import com.mapswithme.maps.api.MWMPoint;

public interface MapDataProvider
{
  public final static double MIN_ZOOM = 1;
  public final static double MAX_ZOOM = 19;
  public final static double ZOOM_DEFAULT = 11;

  MapData getMyPositionData(double lat, double lon, double zoom);
  MapData getPOIData(MWMPoint poi, double zoom);
}
