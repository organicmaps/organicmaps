package app.organicmaps.sdk.routing;

/**
 * Why a follow-track request was refused. Everything but {@link #Ok} leaves the routing session
 * exactly as it was.
 * <p>
 * IMPORTANT: the order MUST be the same as in the native FollowTrackResult enum
 * (libs/map/routing_manager.hpp).
 */
public enum FollowTrackResult
{
  Ok,
  /** The track does not exist (anymore). */
  NoTrack,
  /** The track has no two distinct points to follow. */
  DegenerateTrack,
  /** A profile a track can never be followed with (Transit, Ruler). */
  UnsupportedProfile,
  /** Called before the core was initialized. */
  NotInitialized,
  /** A preview is building or the journey is already active. */
  Busy,
  /** The selected source geometry is unavailable or has changed. */
  InvalidSelection,
  /** No connected track geometry remains in the chosen direction. */
  NoRemainingGeometry;

  public static FollowTrackResult valueOf(int result)
  {
    return FollowTrackResult.values()[result];
  }
}
