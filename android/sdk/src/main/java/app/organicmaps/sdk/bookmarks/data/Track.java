package app.organicmaps.sdk.bookmarks.data;

import androidx.annotation.ColorInt;
import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.sdk.routing.RoutePointInfo;
import app.organicmaps.sdk.util.Distance;
import java.util.Arrays;
import java.util.List;

public final class Track extends MapObject
{
  private final long mId;
  private final boolean mIsRelationTrack;
  private String mName;
  private final Distance mLength;
  private long mCategoryId;
  @ColorInt
  private int mColor;
  @Nullable
  private ElevationInfo mElevationInfo;
  @Nullable
  private TrackStatistics mTrackStatistics;
  @NonNull
  private final List<TrackSelectionCandidate> mCandidates;

  // Called from JNI.
  @Keep
  @SuppressWarnings("unused")
  private Track(long id, long categoryId, boolean isRelationTrack, String name, Distance length, int color)
  {
    super(TRACK, name, "", "", "", 0, 0, "", null, OPENING_MODE_PREVIEW_PLUS, "", "",
          RoadWarningMarkType.UNKNOWN.ordinal(), null);
    mId = id;
    mIsRelationTrack = isRelationTrack;
    mCategoryId = categoryId;
    mName = name;
    mLength = length;
    mColor = color;
    mCandidates = List.of();
  }

  // Called from JNI.
  @Keep
  @SuppressWarnings("unused")
  private Track(long categoryId, long id, boolean isRelationTrack, String title, @Nullable String secondaryTitle,
                @Nullable String subtitle, @Nullable String address, @Nullable RoutePointInfo routePointInfo,
                @OpeningMode int openingMode, @NonNull String wikiArticle, @NonNull String osmDescription,
                @Nullable String[] rawTypes, @ColorInt int color, Distance length, double lat, double lon,
                @Nullable TrackSelectionCandidate[] candidates)
  {
    super(TRACK, title, secondaryTitle, subtitle, address, lat, lon, "", routePointInfo, openingMode, wikiArticle,
          osmDescription, RoadWarningMarkType.UNKNOWN.ordinal(), rawTypes);
    mId = id;
    mIsRelationTrack = isRelationTrack;
    mCategoryId = categoryId;
    mColor = color;
    mName = title;
    mLength = length;
    mCandidates = (candidates != null) ? List.copyOf(Arrays.asList(candidates)) : List.of();
  }

  public long getTrackId()
  {
    return mId;
  }

  public boolean isRelationTrack()
  {
    return mIsRelationTrack;
  }

  public void setCategoryId(long categoryId)
  {
    if (categoryId == mCategoryId)
      return;

    final long oldCatId = mCategoryId;
    mCategoryId = categoryId;
    nativeChangeCategory(oldCatId, mCategoryId, mId);
  }

  @NonNull
  public String getName()
  {
    return mName;
  }

  public Distance getLength()
  {
    return mLength;
  }

  @ColorInt
  public int getColor()
  {
    return mColor;
  }

  public void setColor(@ColorInt int color)
  {
    mColor = color;
    nativeChangeColor(mId, mColor);
  }

  public long getCategoryId()
  {
    return mCategoryId;
  }

  @NonNull
  @Override
  public String getDescription()
  {
    return nativeGetDescription(mId);
  }

  @Nullable
  public ElevationInfo getElevationInfo()
  {
    if (mElevationInfo == null)
      mElevationInfo = nativeGetElevationInfo(mId);
    return mElevationInfo;
  }

  @NonNull
  public TrackStatistics getTrackStatistics()
  {
    if (mTrackStatistics == null)
      mTrackStatistics = nativeGetStatistics(mId);
    return mTrackStatistics;
  }

  @NonNull
  public double[] getElevationActivePointCoordinates()
  {
    return nativeGetElevationActivePointCoordinates(mId);
  }

  public double getElevationCurPositionDistance()
  {
    return nativeGetElevationCurPositionDistance(mId);
  }

  public double getElevationActivePointDistance()
  {
    return nativeGetElevationActivePointDistance(mId);
  }

  @NonNull
  public List<TrackSelectionCandidate> getCandidates()
  {
    return mCandidates;
  }

  public boolean hasMultipleCandidates()
  {
    return mCandidates.size() > 1;
  }

  public void update(@NonNull String name, @ColorInt int color, @NonNull String description)
  {
    if (!name.equals(mName) || !(color == mColor) || !description.equals(getDescription()))
      nativeSetParams(mId, name, color, description);
    mName = name;
    mColor = color;
  }

  @NonNull
  private static native String nativeGetDescription(long id);
  @Nullable
  public static native ElevationInfo nativeGetElevationInfo(long id);
  @NonNull
  public static native TrackStatistics nativeGetStatistics(long id);
  @NonNull
  private static native double[] nativeGetElevationActivePointCoordinates(long trackId);

  private static native void nativeSetParams(long id, @NonNull String name, @ColorInt int color, @NonNull String descr);
  private static native void nativeChangeColor(long id, @ColorInt int color);
  private static native void nativeChangeCategory(long oldCatId, long newCatId, long trackId);

  private static native double nativeGetElevationCurPositionDistance(long trackId);
  private static native double nativeGetElevationActivePointDistance(long trackId);
}
