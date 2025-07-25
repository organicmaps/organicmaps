package app.organicmaps.sdk.bookmarks.data;

import androidx.annotation.IntRange;
import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.sdk.routing.RoutePointInfo;
import app.organicmaps.sdk.search.Popularity;
import app.organicmaps.sdk.util.Distance;

// Called from JNI.
@Keep
@SuppressWarnings("unused")
public class Track extends MapObject
{
  private final long mTrackId;
  private long mCategoryId;
  private final String mName;
  private final Distance mLength;
  private int mColor;
  @Nullable
  private ElevationInfo mElevationInfo;
  @Nullable
  private TrackStatistics mTrackStatistics;

  Track(long trackId, long categoryId, String name, Distance length, int color)
  {
    super(FeatureId.fromFeatureIdString("1:2:3"), TRACK, name, "", "", "", 0, 0, "", null, OPENING_MODE_PREVIEW_PLUS,
          null, "", RoadWarningMarkType.UNKNOWN.ordinal(), null);
    mTrackId = trackId;
    mCategoryId = categoryId;
    mName = name;
    mLength = length;
    mColor = color;
  }

  // used by JNI
  Track(@NonNull FeatureId featureId, @IntRange(from = 0) long categoryId, @IntRange(from = 0) long trackId,
        String title, @Nullable String secondaryTitle, @Nullable String subtitle, @Nullable String address,
        @Nullable RoutePointInfo routePointInfo, @OpeningMode int openingMode, @NonNull Popularity popularity,
        @NonNull String description, @Nullable String[] rawTypes, int color, Distance length, double lat, double lon)
  {
    super(featureId, TRACK, title, secondaryTitle, subtitle, address, lat, lon, "", routePointInfo, openingMode,
          popularity, description, RoadWarningMarkType.UNKNOWN.ordinal(), rawTypes);
    mTrackId = trackId;
    mCategoryId = categoryId;
    mColor = color;
    mName = title;
    mLength = length;
  }

  // modifying this categoryId will not change the core data
  // its just for temporary changes
  public void setCategoryId(@NonNull long categoryId)
  {
    mCategoryId = categoryId;
  }

  public void setColor(@NonNull int color)
  {
    mColor = color;
    BookmarkManager.INSTANCE.changeTrackColor(getTrackId(), color);
  }

  public String getName()
  {
    return mName;
  }

  public Distance getLength()
  {
    return mLength;
  }

  public int getColor()
  {
    return mColor;
  }

  public long getTrackId()
  {
    return mTrackId;
  }

  public long getCategoryId()
  {
    return mCategoryId;
  }

  public String getTrackDescription()
  {
    return BookmarkManager.INSTANCE.getTrackDescription(mTrackId);
  }

  public ElevationInfo getElevationInfo()
  {
    if (mElevationInfo != null)
      return mElevationInfo;
    mElevationInfo = BookmarkManager.nativeGetTrackElevationInfo(mTrackId);
    return mElevationInfo;
  }

  public boolean isElevationInfoHasValue()
  {
    return BookmarkManager.nativeIsElevationInfoHasValue(mTrackId);
  }

  public TrackStatistics getTrackStatistics()
  {
    if (mTrackStatistics != null)
      return mTrackStatistics;
    mTrackStatistics = BookmarkManager.nativeGetTrackStatistics(mTrackId);
    return mTrackStatistics;
  }
}
