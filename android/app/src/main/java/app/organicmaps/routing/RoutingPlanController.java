package app.organicmaps.routing;

import androidx.annotation.NonNull;
import app.organicmaps.sdk.settings.RoadType;

/**
 * Interface for the activity to implement to handle activity-level routing actions
 * requested by the {@link RoutingPlanFragment}.
 */
public interface RoutingPlanController
{
  /** Show the search activity for route point selection */
  void showSearchForRoutePoint();
  /** Show the routing start disclaimer, returns true if accepted */
  boolean showRoutingDisclaimer();
  /** Show the start point notice, returns true if ready to start */
  boolean showStartPointNotice();
  /** Close floating panels */
  void closeFloatingPanels();
  /** Set fullscreen mode */
  void setFullscreen(boolean fullscreen);
  /** Toggle route settings */
  void onPlacePageRequestToggleRouteSettings(@NonNull RoadType roadType);
}
