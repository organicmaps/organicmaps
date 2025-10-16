package app.organicmaps.sdk.bookmarks.data;

import androidx.annotation.IntRange;
import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.util.Distance;
import app.organicmaps.sdk.util.GeoUtils;

// Called from JNI.
@Keep
@SuppressWarnings("unused")
public class BookmarkInfo
{
  private final long mCategoryId;
  private final long mBookmarkId;
  @NonNull
  private String mTitle;
  @NonNull
  private final String mFeatureType;
  @NonNull
  private Icon mIcon;
  private final double mMerX;
  private final double mMerY;
  private final double mScale;
  @NonNull
  private final String mAddress;
  @NonNull
  private final ParcelablePointD mLatLonPoint;

  public BookmarkInfo(@IntRange(from = 0) long categoryId, @IntRange(from = 0) long bookmarkId)
  {
    mCategoryId = categoryId;
    mBookmarkId = bookmarkId;
    mTitle = Bookmark.nativeGetName(mBookmarkId);
    mFeatureType = Bookmark.nativeGetFeatureType(mBookmarkId);
    mIcon = new Icon(Bookmark.nativeGetColor(mBookmarkId), Bookmark.nativeGetIcon(mBookmarkId));
    final ParcelablePointD ll = Bookmark.nativeGetXY(mBookmarkId);
    mMerX = ll.x;
    mMerY = ll.y;
    mScale = Bookmark.nativeGetScale(mBookmarkId);
    mAddress = Bookmark.nativeGetAddress(mBookmarkId);
    mLatLonPoint = GeoUtils.toLatLon(mMerX, mMerY);
  }

  public long getCategoryId()
  {
    return mCategoryId;
  }

  public long getBookmarkId()
  {
    return mBookmarkId;
  }

  public DistanceAndAzimut getDistanceAndAzimuth(double cLat, double cLon, double north)
  {
    return Framework.nativeGetDistanceAndAzimuth(mMerX, mMerY, cLat, cLon, north);
  }

  @NonNull
  public String getFeatureType()
  {
    return mFeatureType;
  }

  @NonNull
  public String getName()
  {
    return mTitle;
  }

  @NonNull
  public Icon getIcon()
  {
    return mIcon;
  }

  @NonNull
  public Distance getDistance(double latitude, double longitude, double v)
  {
    return getDistanceAndAzimuth(latitude, longitude, v).getDistance();
  }

  public double getLat()
  {
    return mLatLonPoint.x;
  }

  public double getLon()
  {
    return mLatLonPoint.y;
  }

  public double getScale()
  {
    return mScale;
  }

  @NonNull
  public String getAddress()
  {
    return mAddress;
  }

  @NonNull
  public String getDescription()
  {
    return Bookmark.nativeGetDescription(mBookmarkId);
  }

  public void update(@NonNull String name, @Nullable Icon icon, @NonNull String description)
  {
    if (icon == null)
      icon = getIcon();

    if (!name.equals(getName()) || !icon.equals(getIcon()) || !description.equals(getDescription()))
      Bookmark.nativeUpdateParams(getBookmarkId(), name, icon.getColor(), description);
    mIcon = icon;
    mTitle = name;
  }

  public void changeCategory(@IntRange(from = 0) long newCategoryId)
  {
    Bookmark.nativeChangeCategory(mCategoryId, newCategoryId, mBookmarkId);
  }
}
