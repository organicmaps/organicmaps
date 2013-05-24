package com.mapswithme.maps.api;

import java.io.Serializable;

// TODO add javadoc for public interface
public final class MWMPoint implements Serializable
{
  private static final long serialVersionUID = 1L;

  final private double mLat;
  final private double mLon;
  final private String mName;
  private String mId;

  public MWMPoint(double lat, double lon, String name)
  {
    this(lat, lon, name, null);
  }

  public MWMPoint(double lat, double lon, String name, String id)
  {
    this.mLat = lat;
    this.mLon = lon;
    this.mName = name;
    this.mId = id;
  }

  public double getLat()       { return mLat;   }
  public double getLon()       { return mLon;   }
  public String getName()      { return mName;  }
  public String getId()        { return mId;    }

  public void setId(String id) { mId = id; }

  @Override
  public String toString()
  {
    return "MWMPoint [mLat=" + mLat + ", mLon=" + mLon + ", mName=" + mName + ", mId=" + mId + "]";
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
    final MWMPoint other = (MWMPoint) obj;
    if (Double.doubleToLongBits(mLat) != Double.doubleToLongBits(other.mLat))
      return false;
    if (Double.doubleToLongBits(mLon) != Double.doubleToLongBits(other.mLon))
      return false;
    if (mName == null)
    {
      if (other.mName != null)
        return false;
    } else if (!mName.equals(other.mName))
      return false;
    return true;
  }
}
