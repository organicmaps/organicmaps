package com.mapswithme.yopme.map;

import android.content.Context;

import com.mapswithme.yopme.PoiPoint;


public interface MapDataProvider
{
  public final static double MIN_ZOOM = 1;
  public final static double MAX_ZOOM = 19;
  public final static double ZOOM_DEFAULT = 11;
  public final static double COMFORT_ZOOM = 17;

  MapData getMapData(Context context, PoiPoint viewPortCenter,
                     double zoom, PoiPoint poi, PoiPoint myLocation);
}
