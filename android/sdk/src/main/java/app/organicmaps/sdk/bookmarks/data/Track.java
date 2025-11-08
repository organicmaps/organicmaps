package app.organicmaps.sdk.bookmarks.data;

import androidx.annotation.ColorInt;
import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.sdk.routing.RoutePointInfo;
import app.organicmaps.sdk.util.Distance;

public final class Track extends MapObject
{
  private final long mId;
  private String mName;
  private final Distance mLength;
  private long mCategoryId;
  @ColorInt
  private int mColor;
  @Nullable
  private ElevationInfo mElevationInfo;
  @Nullable
  private TrackStatistics mTrackStatistics;

  // Called from JNI.
  @Keep
  @SuppressWarnings("unused")
  private Track(long id, long categoryId, String name, Distance length, int color)
  {
    super(TRACK, name, "", "", "", 0, 0, "", null, OPENING_MODE_PREVIEW_PLUS, "", "",
          RoadWarningMarkType.UNKNOWN.ordinal(), null);
    mId = id;
    mCategoryId = categoryId;
    mName = name;
    mLength = length;
    mColor = color;
  }

  // Called from JNI.
  @Keep
  @SuppressWarnings("unused")
  private Track(long categoryId, long id, String title, @Nullable String secondaryTitle, @Nullable String subtitle,
                @Nullable String address, @Nullable RoutePointInfo routePointInfo, @OpeningMode int openingMode,
                @NonNull String wikiArticle, @NonNull String osmDescription, @Nullable String[] rawTypes,
                @ColorInt int color, Distance length, double lat, double lon)
  {
    super(TRACK, title, secondaryTitle, subtitle, address, lat, lon, "", routePointInfo, openingMode, wikiArticle,
          osmDescription, RoadWarningMarkType.UNKNOWN.ordinal(), rawTypes);
    mId = id;
    mCategoryId = categoryId;
    mColor = color;
    mName = title;
    mLength = length;
  }

  public long getTrackId()
  {
    return mId;
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
  public ElevationInfo.Point getElevationActivePointCoordinates()
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
  private static native ElevationInfo.Point nativeGetElevationActivePointCoordinates(long trackId);

  private static native void nativeSetParams(long id, @NonNull String name, @ColorInt int color, @NonNull String descr);
  private static native void nativeChangeColor(long id, @ColorInt int color);
  private static native void nativeChangeCategory(long oldCatId, long newCatId, long trackId);

  private static native double nativeGetElevationCurPositionDistance(long trackId);
  private static native double nativeGetElevationActivePointDistance(long trackId);
}
