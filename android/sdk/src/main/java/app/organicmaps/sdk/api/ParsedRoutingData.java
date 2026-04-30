package app.organicmaps.sdk.api;

import androidx.annotation.Keep;
import app.organicmaps.sdk.Router;

/**
 * Represents Framework::ParsedRoutingData from core.
 */
// Used by JNI.
@Keep
@SuppressWarnings("unused")
public class ParsedRoutingData
{
  public final RoutePoint[] mPoints;
  public final Router mRouterType;
  public final boolean mOptimizeRoutePoints;
  public final boolean mStartRouteNavigation;
  public final double mStartDirectionX;
  public final double mStartDirectionY;

  public ParsedRoutingData(RoutePoint[] points, int routerType, boolean optimizeRoutePoints,
                           boolean startRouteNavigation, double startDirectionX, double startDirectionY)
  {
    this.mPoints = points;
    this.mRouterType = Router.valueOf(routerType);
    this.mOptimizeRoutePoints = optimizeRoutePoints;
    this.mStartRouteNavigation = startRouteNavigation;
    this.mStartDirectionX = startDirectionX;
    this.mStartDirectionY = startDirectionY;
  }
}
