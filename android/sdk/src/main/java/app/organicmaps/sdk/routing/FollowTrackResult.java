package app.organicmaps.sdk.routing;

/**
 * Result of a follow-track request. An accepted chart inspection is reported by
 * {@link RoutingController#updateFollowedTrackEntryAt(double)} as {@link #Ok}.
 * <p>
 * IMPORTANT: the order MUST be the same as in the native FollowTrackResult enum
 * (libs/map/routing_manager.hpp).
 */
public enum FollowTrackResult
{
  Ok,
  /** The requested source track does not exist. */
  NoTrack,
  /** The current build or navigation state prevents this operation. */
  Busy,
  /** The selected source geometry is unavailable or has changed. */
  InvalidSelection,
  /** No connected track geometry remains in this direction. */
  NoRemainingGeometry,
  /** The core accepted the edit without starting a build. */
  Unchanged;

  public static FollowTrackResult valueOf(int result)
  {
    return FollowTrackResult.values()[result];
  }
}
