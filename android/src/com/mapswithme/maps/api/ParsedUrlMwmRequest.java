package com.mapswithme.maps.api;

import android.support.annotation.IntDef;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * Represents url_scheme::ParsedMapApi::ParsingResult from core.
 */
public class ParsedUrlMwmRequest
{
  @Retention(RetentionPolicy.SOURCE)
  @IntDef({RESULT_INCORRECT, RESULT_MAP, RESULT_ROUTE, RESULT_SEARCH, RESULT_LEAD})
  public @interface ParsingResult {}

  public static final int RESULT_INCORRECT = 0;
  public static final int RESULT_MAP = 1;
  public static final int RESULT_ROUTE = 2;
  public static final int RESULT_SEARCH = 3;
  public static final int RESULT_LEAD = 4;

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
