package app.organicmaps.sdk.routing;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;

/** Captured track details remain valid after the source is edited or deleted. */
// Called from JNI.
@Keep
@SuppressWarnings("unused")
public final class FollowedTrackInfo
{
  private final long mTrackId;
  @NonNull
  private final String mName;
  private final double mEntryDistanceMeters;
  private final boolean mReversed;
  private final boolean mCanReverse;
  private final boolean mEndsAtGap;

  private FollowedTrackInfo(long trackId, @NonNull String name, double entryDistanceMeters, boolean reversed,
                            boolean canReverse, boolean endsAtGap)
  {
    mTrackId = trackId;
    mName = name;
    mEntryDistanceMeters = entryDistanceMeters;
    mReversed = reversed;
    mCanReverse = canReverse;
    mEndsAtGap = endsAtGap;
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

  /** Captured distance along the source track, unaffected by later map selections. */
  public double getEntryDistanceMeters()
  {
    return mEntryDistanceMeters;
  }

  /** Whether the selected entry supports travel in the opposite direction. */
  public boolean canReverse()
  {
    return mCanReverse;
  }

  /** Whether the destination borders a gap inside the recorded track. */
  public boolean endsAtGap()
  {
    return mEndsAtGap;
  }

  /** Whether travel runs toward the start of the recorded section. */
  public boolean isReversed()
  {
    return mReversed;
  }
}
