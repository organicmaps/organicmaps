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
  /** The requested source track does not exist. */
  NoTrack,
  /** The current build or navigation state prevents this operation. */
  Busy,
  /** The selected source geometry is unavailable or has changed. */
  InvalidSelection,
  /** No connected track geometry remains in this direction. */
  NoRemainingGeometry,
  /** The saved track profile supports measurement only. */
  UnsupportedProfile;

  public static FollowTrackResult valueOf(int result)
  {
    return FollowTrackResult.values()[result];
  }
}
