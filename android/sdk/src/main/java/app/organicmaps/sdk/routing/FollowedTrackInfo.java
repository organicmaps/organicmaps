package app.organicmaps.sdk.routing;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;

/**
 * The id and name of the currently followed track. The session may outlive its track --
 * navigation continues on the frozen geometry when the user deletes the track mid-session -- so
 * this info stays valid afterwards.
 */
// Called from JNI.
@Keep
@SuppressWarnings("unused")
public final class FollowedTrackInfo
{
  private final long mTrackId;
  @NonNull
  private final String mName;

  private FollowedTrackInfo(long trackId, @NonNull String name)
  {
    mTrackId = trackId;
    mName = name;
  }

  /** The track may have been deleted meanwhile: resolving it needs an existence check. */
  public long getTrackId()
  {
    return mTrackId;
  }

  @NonNull
  public String getName()
  {
    return mName;
  }
}
