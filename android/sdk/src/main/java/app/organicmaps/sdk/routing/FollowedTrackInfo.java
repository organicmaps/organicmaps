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

  private FollowedTrackInfo(long trackId, @NonNull String name, double entryDistanceMeters)
  {
    mTrackId = trackId;
    mName = name;
    mEntryDistanceMeters = entryDistanceMeters;
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
}
