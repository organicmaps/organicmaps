package app.organicmaps.routing;

import androidx.annotation.Keep;
import androidx.annotation.Nullable;

import app.organicmaps.util.Distance;

/**
 * Represents RouteMarkData from core.
 */
// Called from JNI.
@Keep
@SuppressWarnings("unused")
public class RouteMarkData
{
  @Nullable
  public final String mTitle;
  @Nullable
  public final String mSubtitle;
  @RoutePointInfo.RouteMarkType
  public final int mPointType;
  public final int mIntermediateIndex;
  public final boolean mIsVisible;
  public final boolean mIsMyPosition;
  public final boolean mIsPassed;
  public final double mLat;
  public final double mLon;
  public final long mTimeSec;
  public final String mDistance;

  public RouteMarkData(@Nullable String title, @Nullable String subtitle,
                       @RoutePointInfo.RouteMarkType int pointType,
                       int intermediateIndex, boolean isVisible, boolean isMyPosition,
                       boolean isPassed, double lat, double lon, long timeSec,
                       String distance)
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
    mTimeSec = timeSec;
    mDistance = distance;
  }
}
