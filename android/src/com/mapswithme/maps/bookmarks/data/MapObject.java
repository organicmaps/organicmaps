package com.mapswithme.maps.bookmarks.data;

import com.mapswithme.maps.MapObjectFragment.MapObjectType;

public abstract class MapObject
{
  public static double DEF_SCALE = 14.0;
  public abstract double getScale();
  public abstract String getName();
  public abstract double getLat();
  public abstract double getLon();
  
  public abstract MapObjectType getType();
}
