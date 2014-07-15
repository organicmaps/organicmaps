package com.mapswithme.maps.bookmarks.data;

import android.content.res.Resources;
import android.text.TextUtils;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.R;

import java.io.Serializable;

public abstract class MapObject
{
  protected String mName;
  protected double mLat;
  protected double mLon;
  protected String mTypeName;

  public MapObject(String name, double lat, double lon, String typeName)
  {
    mName = name;
    mLat = lat;
    mLon = lon;
    mTypeName = typeName;
  }

  private static boolean isEmpty(String s)
  {
    return (s == null || s.length() == 0);
  }

  public void setDefaultIfEmpty(Resources res)
  {
    if (isEmpty(mName))
    {
      if (isEmpty(mTypeName))
        mName = res.getString(R.string.dropped_pin);
      else
        mName = mTypeName;
    }

    if (isEmpty(mTypeName))
      mTypeName = res.getString(R.string.placepage_unsorted);
  }

  @Override
  public int hashCode()
  {
    final int prime = 31;
    int result = 1;
    long temp;
    temp = Double.doubleToLongBits(mLat);
    result = prime * result + (int) (temp ^ (temp >>> 32));
    temp = Double.doubleToLongBits(mLon);
    result = prime * result + (int) (temp ^ (temp >>> 32));
    result = prime * result + ((mName == null) ? 0 : mName.hashCode());
    result = prime * result + ((mTypeName == null) ? 0 : mTypeName.hashCode());
    return result;
  }

  @Override
  public boolean equals(Object obj)
  {
    if (this == obj)
      return true;
    if (obj == null)
      return false;
    if (getClass() != obj.getClass())
      return false;
    final MapObject other = (MapObject) obj;
    if (Double.doubleToLongBits(mLat) != Double.doubleToLongBits(other.mLat))
      return false;
    if (Double.doubleToLongBits(mLon) != Double.doubleToLongBits(other.mLon))
      return false;
    if (mName == null)
    {
      if (other.mName != null)
        return false;
    }
    else if (!mName.equals(other.mName))
      return false;
    if (mTypeName == null)
    {
      if (other.mTypeName != null)
        return false;
    }
    else if (!mTypeName.equals(other.mTypeName))
      return false;
    return true;
  }

  public double getScale() { return 0; };
  public String getName()  { return mName; }
  public double getLat()   { return mLat; }
  public double getLon()   { return mLon; }
  public String getPoiTypeName() { return mTypeName; }

  public abstract MapObjectType getType();

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
    public Poi(String name, double lat, double lon, String typeName)
    {
      super(name, lat, lon, typeName);
    }

    @Override
    public MapObjectType getType()
    {
      return MapObjectType.POI;
    }
  }

  public static class SearchResult extends MapObject
  {
    public SearchResult(long index)
    {
      super("", 0, 0, "");
      Framework.injectData(this, index);
    }

    public SearchResult(String name, String type, double lat, double lon)
    {
      super(name, lat, lon, type);
    }

    @Override
    public MapObjectType getType()
    {
      return MapObjectType.ADDITIONAL_LAYER;
    }
  }

  public static class ApiPoint extends MapObject
  {
    private final String mId;

    public ApiPoint(String name, String id, String poiType, double lat, double lon)
    {
      super(name, lat, lon, poiType);
      mId = id;
    }

    @Override
    public MapObjectType getType()
    {
      return MapObjectType.API_POINT;
    }

    public String getId()
    {
      return mId;
    }
  }

  public static class MyPosition extends MapObject
  {


    public MyPosition(String name, double lat, double lon)
    {
      super(name, lat, lon, "");
    }

    @Override
    public MapObjectType getType()
    {
      return MapObjectType.MY_POSITION;
    }

    @Override
    public void setDefaultIfEmpty(Resources res)
    {
      if (TextUtils.isEmpty(mName))
      {
        mName = res.getString(R.string.my_position);
      }
    }
  }

}
