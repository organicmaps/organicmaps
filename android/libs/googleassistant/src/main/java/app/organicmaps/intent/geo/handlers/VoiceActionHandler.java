package app.organicmaps.intent.geo.handlers;

import app.organicmaps.sdk.util.log.Logger;

public interface VoiceActionHandler
{
  String TAG = VoiceActionHandler.class.getSimpleName();

  /**
   * Enables or disables navigation voice guidance.
   *
   * @param enable {@code true} to enable voice guidance, {@code false} to mute it.
   */
  default void processVoiceGuidanceAction(boolean enable)
  {
    Logger.d(TAG, "Not implemented: process voice guidance action enable=" + enable);
  }

  /** Speaks the current traffic report. */
  default void speakTrafficReport()
  {
    Logger.d(TAG, "Not implemented: traffic report");
  }

  /** Speaks the current road name. */
  default void speakCurrentRoad()
  {
    Logger.d(TAG, "Not implemented: query current road");
  }

  /** Speaks the current destination name during navigation. */
  default void speakDestination()
  {
    Logger.d(TAG, "Not implemented: query destination");
  }

  /** Speaks the estimated time of arrival during navigation. */
  default void speakEta()
  {
    Logger.d(TAG, "Not implemented: show ETA to destination");
  }

  /** Speaks remaining distance to destination during navigation. */
  default void speakDistanceToDestination()
  {
    Logger.d(TAG, "Not implemented: show distance to destination");
  }

  /** Speaks remaining travel time to destination during navigation. */
  default void speakTimeToDestination()
  {
    Logger.d(TAG, "Not implemented: show time to destination");
  }

  /** Speaks the next maneuver during navigation. */
  default void speakNextTurn()
  {
    Logger.d(TAG, "Not implemented: query next turn");
  }

  /** Speaks distance to the next maneuver during navigation. */
  default void speakDistanceToNextTurn()
  {
    Logger.d(TAG, "Not implemented: show distance to next turn");
  }

  /** Speaks time remaining to the next maneuver during navigation. */
  default void speakTimeToNextTurn()
  {
    Logger.d(TAG, "Not implemented: show time to next turn");
  }
}
