package com.mapswithme.maps.bookmarks.data;

import java.io.Serializable;

import com.mapswithme.maps.Framework;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.SimpleLogger;

public abstract class MapObject
{
  private static final Logger mLog = SimpleLogger.get("MwmMapObject");

  public double getScale() { return 0; };
  // Interface
  public abstract String getName();
  public abstract double getLat();
  public abstract double getLon();

  public abstract MapObjectType getType();
  // interface

  public static Integer checkSum(MapObject mo)
  {
    if (mo == null) return 0;

    final int[] primes = {2, 3, 5, 7, 11, 13, 17, 19, 23};

    final int base = primes[mo.getType().ordinal()];
    final int component1 = Double.valueOf(mo.getLat()).hashCode();
    final int component2 = Double.valueOf(mo.getLon()).hashCode();
    final int component3 = mo.getName() == null ? base : mo.getName().hashCode();

    final int sum = (component1 << base)
                  + (component2 >> base)
                  +  component3;

    mLog.d(String.format("c1=%d c2=%d c3=%d sum=%d", component1, component2, component3, sum));

    return sum;
  }

  public static enum MapObjectType implements Serializable
  {
    POI,
    API_POINT,
    BOOKMARK,
    MY_POSITION,
    ADDITIONAL_LAYER
  }

  public static class Poi extends MapObject
  {
    private final String mName;
    private final double mLat;
    private final double mLon;
    private final String mCategory;

    public Poi(String name, double lat, double lon, String category)
    {
      mName = name;
      mLat = lat;
      mLon = lon;
      mCategory = category;
    }

    @Override
    public String getName()
    {
      return mName;
    }

    @Override
    public double getLat()
    {
      return mLat;
    }

    @Override
    public double getLon()
    {
      return mLon;
    }

    @Override
    public MapObjectType getType()
    {
      return MapObjectType.POI;
    }

    public String getCategory()
    {
      return mCategory;
    }
  }

  public static class SearchResult extends MapObject
  {
    private String mName;
    private double mLat;
    private double mLon;

    public SearchResult(long index)
    {
      Framework.injectData(this, index);
    }

    @Override
    public String getName()
    {
      return mName;
    }

    @Override
    public double getLat()
    {
      return mLat;
    }

    @Override
    public double getLon()
    {
      return mLon;
    }

    @Override
    public MapObjectType getType()
    {
      return MapObjectType.ADDITIONAL_LAYER;
    }

  }
}
