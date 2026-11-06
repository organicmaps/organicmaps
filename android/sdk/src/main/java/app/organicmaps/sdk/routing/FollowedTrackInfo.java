package app.organicmaps.sdk.routing;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;

/**
 * Everything the UI needs about the currently followed track. A snapshot taken when the follow
 * started: the session may outlive its track -- navigation continues on the frozen geometry when
 * the user deletes the track mid-session -- so this info stays valid and correct afterwards.
 */
// Called from JNI.
@Keep
@SuppressWarnings("unused")
public final class FollowedTrackInfo
{
  private final long mTrackId;
  @NonNull
  private final String mName;
  private final double mLengthMeters;
  private final boolean mReversed;

  private FollowedTrackInfo(long trackId, @NonNull String name, double lengthMeters, boolean reversed)
  {
    mTrackId = trackId;
    mName = name;
    mLengthMeters = lengthMeters;
    mReversed = reversed;
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

  public double getLengthMeters()
  {
    return mLengthMeters;
  }

  public boolean isReversed()
  {
    return mReversed;
  }
}
