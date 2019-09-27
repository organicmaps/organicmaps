package com.mapswithme.maps.bookmarks.data;

import androidx.annotation.IntRange;
import androidx.annotation.NonNull;
import com.mapswithme.maps.Framework;
import com.mapswithme.util.GeoUtils;
import com.mapswithme.util.sharing.ShareableInfoProvider;

public class BookmarkInfo implements ShareableInfoProvider
{
  private final long mCategoryId;
  private final long mBookmarkId;
  @NonNull
  private final String mTitle;
  @NonNull
  private final String mFeatureType;
  @NonNull
  private final Icon mIcon;
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
    mTitle = BookmarkManager.INSTANCE.getBookmarkName(mBookmarkId);
    mFeatureType = BookmarkManager.INSTANCE.getBookmarkFeatureType(mBookmarkId);
    mIcon = new Icon(BookmarkManager.INSTANCE.getBookmarkColor(mBookmarkId),
                     BookmarkManager.INSTANCE.getBookmarkIcon(mBookmarkId));
    final ParcelablePointD ll = BookmarkManager.INSTANCE.getBookmarkXY(mBookmarkId);
    mMerX = ll.x;
    mMerY = ll.y;
    mScale = BookmarkManager.INSTANCE.getBookmarkScale(mBookmarkId);
    mAddress = BookmarkManager.INSTANCE.getBookmarkAddress(mBookmarkId);
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
  public String getFeatureType() { return mFeatureType; }

  @NonNull
  @Override
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
  public String getDistance(double latitude, double longitude, double v)
  {
    return getDistanceAndAzimuth(latitude, longitude, v).getDistance();
  }

  @Override
  public double getLat()
  {
    return mLatLonPoint.x;
  }

  @Override
  public double getLon()
  {
    return mLatLonPoint.y;
  }

  @Override
  public double getScale()
  {
    return mScale;
  }

  @NonNull
  @Override
  public String getAddress()
  {
    return mAddress;
  }
}
