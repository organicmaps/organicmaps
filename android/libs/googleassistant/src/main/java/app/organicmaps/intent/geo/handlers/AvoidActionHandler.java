package app.organicmaps.intent.geo.handlers;

import app.organicmaps.sdk.util.log.Logger;

public interface AvoidActionHandler
{
  String TAG = AvoidActionHandler.class.getSimpleName();

  /**
   * Updates the ferries preference for route planning.
   *
   * <p>This may trigger route recalculation.
   *
   * @param allow {@code true} to allow ferries, {@code false} to avoid ferries.
   */
  default void processFerriesAction(boolean allow)
  {
    Logger.d(TAG, "Not implemented: process ferries action allow=" + allow);
  }

  /**
   * Updates the highways preference for route planning.
   *
   * <p>This may trigger route recalculation.
   *
   * @param allow {@code true} to allow highways, {@code false} to avoid highways.
   */
  default void processHighwaysAction(boolean allow)
  {
    Logger.d(TAG, "Not implemented: process highways action allow=" + allow);
  }

  /**
   * Updates the tolls preference for route planning.
   *
   * <p>This may trigger route recalculation.
   *
   * @param allow {@code true} to allow toll roads, {@code false} to avoid toll roads.
   */
  default void processTollsAction(boolean allow)
  {
    Logger.d(TAG, "Not implemented: process tolls action allow=" + allow);
  }
}
