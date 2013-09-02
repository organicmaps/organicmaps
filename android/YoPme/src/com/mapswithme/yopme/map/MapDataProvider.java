package com.mapswithme.yopme.map;

import com.mapswithme.maps.api.MWMPoint;

public interface MapDataProvider
{
  MapData getMyPositionData(double lat, double lon, double zoom);
  MapData getPOIData(MWMPoint poi, double zoom);
}
