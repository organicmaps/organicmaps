package com.mapswithme.maps.api;

import androidx.annotation.IntDef;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

public class ParsedUrlMwmRequest
{
  public final RoutePoint[] mRoutePoints;
  public final String mGlobalUrl;
  public final String mAppTitle;
  public final int mVersion;
  public final double mZoomLevel;
  public final boolean mGoBackOnBalloonClick;
  public final boolean mIsValid;

  public ParsedUrlMwmRequest(RoutePoint[] routePoints, String globalUrl, String appTitle, int version, double zoomLevel, boolean goBackOnBalloonClick, boolean isValid) {
    this.mRoutePoints = routePoints;
    this.mGlobalUrl = globalUrl;
    this.mAppTitle = appTitle;
    this.mVersion = version;
    this.mZoomLevel = zoomLevel;
    this.mGoBackOnBalloonClick = goBackOnBalloonClick;
    this.mIsValid = isValid;
  }
}
