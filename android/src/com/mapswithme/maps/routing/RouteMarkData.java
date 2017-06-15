package com.mapswithme.maps.routing;

/**
 * Represents RouteMarkData from core.
 */
public class RouteMarkData
{
  public final String mName;
  @RoutePointInfo.RouteMarkType
  public final int mPointType;
  public final int mIntermediateIndex;
  public final boolean mIsVisible;
  public final boolean mIsMyPosition;
  public final boolean mIsPassed;
  public final double mLat;
  public final double mLon;

  public RouteMarkData(String name, @RoutePointInfo.RouteMarkType int pointType,
                       int intermediateIndex, boolean isVisible, boolean isMyPosition,
                       boolean isPassed, double lat, double lon)
  {
    mName = name;
    mPointType = pointType;
    mIntermediateIndex = intermediateIndex;
    mIsVisible = isVisible;
    mIsMyPosition = isMyPosition;
    mIsPassed = isPassed;
    mLat = lat;
    mLon = lon;
  }
}
