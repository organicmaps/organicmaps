package app.organicmaps.routing;

/**
 * Interface for the activity to implement to handle activity-level routing actions
 * requested by the {@link RoutingPlanFragment}.
 */
public interface RoutingPlanController
{
  /** Show the routing start disclaimer, returns true if accepted */
  boolean showRoutingDisclaimer();
  /** Show the start point notice, returns true if ready to start */
  boolean showStartPointNotice();
  /** Close floating panels */
  void closeFloatingPanels();
  /** Set fullscreen mode */
  void setFullscreen(boolean fullscreen);
  /** Handle a back/close press coming from the routing plan UI; returns true if it was consumed */
  boolean handleBackPress();
}
