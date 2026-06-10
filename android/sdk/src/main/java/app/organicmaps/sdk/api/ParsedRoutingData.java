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
  public final Router mRouterType;
  public final boolean mStartRouteNavigation;

  public ParsedRoutingData(int routerType, boolean startRouteNavigation)
  {
    this.mRouterType = Router.valueOf(routerType);
    this.mStartRouteNavigation = startRouteNavigation;
  }
}
