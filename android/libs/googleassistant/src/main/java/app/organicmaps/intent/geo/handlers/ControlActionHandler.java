package app.organicmaps.intent.geo.handlers;

import app.organicmaps.sdk.util.log.Logger;

public interface ControlActionHandler
{
  String TAG = ControlActionHandler.class.getSimpleName();

  /** Cancels active navigation guidance. */
  default void exitNavigation()
  {
    Logger.d(TAG, "Not implemented: exit navigation");
  }

  /** Switches the map to follow mode. */
  default void changeMapViewToFollowMode()
  {
    Logger.d(TAG, "Not implemented: change map view to follow mode");
  }

  /** Returns to the previous screen. */
  default void goBack()
  {
    Logger.d(TAG, "Not implemented: go back");
  }

  /**
   * Enables or disables satellite map style.
   *
   * @param show {@code true} to enable satellite imagery, {@code false} to hide it.
   */
  default void processShowSatelliteAction(boolean show)
  {
    Logger.d(TAG, "Not implemented: process show satellite action show=" + show);
  }

  /**
   * Enables or disables traffic overlay.
   *
   * @param show {@code true} to show traffic, {@code false} to hide traffic.
   */
  default void processShowTrafficAction(boolean show)
  {
    Logger.d(TAG, "Not implemented: process show traffic action show=" + show);
  }

  /** Opens route overview for the active route. */
  default void showRouteOverview()
  {
    Logger.d(TAG, "Not implemented: show route overview");
  }

  /** Opens available alternative routes for the active route. */
  default void showAlternativeRoutes()
  {
    Logger.d(TAG, "Not implemented: show alternative routes");
  }

  /** Opens the turn-by-turn directions list for the active route. */
  default void showDirectionsList()
  {
    Logger.d(TAG, "Not implemented: show directions list");
  }

  /** Recenters the map on the current user location. */
  default void showMyLocation()
  {
    Logger.d(TAG, "Not implemented: show my location");
  }

  /** Closes open cards and returns to the map. */
  default void showMap()
  {
    Logger.d(TAG, "Not implemented: show map");
  }
}
