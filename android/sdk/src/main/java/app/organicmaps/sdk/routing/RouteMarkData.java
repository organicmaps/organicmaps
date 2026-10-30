package app.organicmaps.sdk.routing;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import java.util.Objects;

/**
 * Immutable native point snapshot. The role and index describe the itinerary at capture time.
 */
// Called from JNI.
@Keep
@SuppressWarnings("unused")
public final class RouteMarkData
{
  @Nullable
  public final String mTitle;
  @Nullable
  public final String mSubtitle;
  public final RouteMarkType mPointType;
  public final int mIntermediateIndex;
  public final boolean mIsVisible;
  public final boolean mIsMyPosition;
  public final boolean mIsPassed;
  public final double mLat;
  public final double mLon;

  private RouteMarkData(@Nullable String title, @Nullable String subtitle, int pointType, int intermediateIndex,
                        boolean isVisible, boolean isMyPosition, boolean isPassed, double lat, double lon)
  {
    this(title, subtitle, RouteMarkType.values()[pointType], intermediateIndex, isVisible, isMyPosition, isPassed, lat,
         lon);
  }

  public RouteMarkData(@Nullable String title, @Nullable String subtitle, RouteMarkType pointType,
                       int intermediateIndex, boolean isVisible, boolean isMyPosition, boolean isPassed, double lat,
                       double lon)
  {
    mTitle = title;
    mSubtitle = subtitle;
    mPointType = pointType;
    mIntermediateIndex = intermediateIndex;
    mIsVisible = isVisible;
    mIsMyPosition = isMyPosition;
    mIsPassed = isPassed;
    mLat = lat;
    mLon = lon;
  }

  /** Compares captured values when checking whether a deferred itinerary edit is still current. */
  public boolean hasSameData(@NonNull RouteMarkData other)
  {
    return (mPointType == other.mPointType && mIntermediateIndex == other.mIntermediateIndex
            && mIsVisible == other.mIsVisible && mIsMyPosition == other.mIsMyPosition && mIsPassed == other.mIsPassed
            && Double.compare(mLat, other.mLat) == 0 && Double.compare(mLon, other.mLon) == 0
            && Objects.equals(mTitle, other.mTitle) && Objects.equals(mSubtitle, other.mSubtitle));
  }
}
