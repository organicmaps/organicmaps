package com.mapswithme.maps.api;

import androidx.annotation.Keep;

import com.mapswithme.maps.Framework;

/**
 * Represents Framework::ParsedRoutingData from core.
 */
@Keep
public class ParsedRoutingData
{
  public final RoutePoint[] mPoints;
  @Framework.RouterType
  public final int mRouterType;

  public ParsedRoutingData(RoutePoint[] points, int routerType) {
    this.mPoints = points;
    this.mRouterType = routerType;
  }
}
