package com.mapswithme.maps.bookmarks.data;

import android.support.annotation.IntRange;
import android.support.annotation.NonNull;

import com.mapswithme.maps.Framework;

public class BookmarkInfo
{
  private final long mCategoryId;
  private final long mBookmarkId;
  @NonNull
  private String mTitle;
  @NonNull
  private Icon mIcon;
  private double mMerX;
  private double mMerY;

  public BookmarkInfo(@IntRange(from = 0) long categoryId, @IntRange(from = 0) long bookmarkId)
  {
    mCategoryId = categoryId;
    mBookmarkId = bookmarkId;
    mTitle = Bookmark.nativeGetName(mBookmarkId);
    mIcon = BookmarkManager.INSTANCE.getIconByColor(Bookmark.nativeGetColor(mBookmarkId));
    final ParcelablePointD ll = Bookmark.nativeGetXY(mBookmarkId);
    mMerX = ll.x;
    mMerY = ll.y;
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
  public String getTitle()
  {
    return mTitle;
  }

  @NonNull
  public Icon getIcon()
  {
    return mIcon;
  }
}
