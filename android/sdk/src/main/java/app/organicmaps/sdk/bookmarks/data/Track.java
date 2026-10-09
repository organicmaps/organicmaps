package app.organicmaps.sdk.bookmarks.data;

import androidx.annotation.ColorInt;
import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
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
  private boolean mVisible;

  // Called from JNI.
  @Keep
  private Track(long categoryId, long id, boolean isRelationTrack, String title, @Nullable String secondaryTitle,
                @NonNull String subtitle, @NonNull String address, @ColorInt int color, Distance length, double lat,
                double lon, @Nullable TrackSelectionCandidate[] candidates, boolean visible)
  {
    super(TRACK, title, secondaryTitle, subtitle, address, lat, lon);
    mId = id;
    mIsRelationTrack = isRelationTrack;
    mCategoryId = categoryId;
    mColor = color;
    mName = title;
    mLength = length;
    mCandidates = (candidates != null) ? List.copyOf(Arrays.asList(candidates)) : List.of();
    mVisible = visible;
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

  public boolean isVisible()
  {
    return mVisible;
  }

  public void setVisibility(boolean visible)
  {
    if (mVisible == visible)
      return;
    mVisible = visible;
    BookmarkManager.INSTANCE.setTrackVisibility(mId, mVisible);
  }

  public void toggleVisibility()
  {
    setVisibility(!mVisible);
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
