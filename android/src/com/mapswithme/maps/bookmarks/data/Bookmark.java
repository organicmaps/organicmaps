package com.mapswithme.maps.bookmarks.data;

import android.content.Context;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.R;

public class Bookmark extends MapObject
{
  private final Icon mIcon;
  private int mCategoryId;
  private int mBookmark;
  private double mMerX;
  private double mMerY;
  private double mLon;
  private double mLat;

  //{@ Populate on creation or lazily?
  private String mName;
  //{@


  /* package */ Bookmark(int categoryId, int bookmarkId, String name)
  {
    mCategoryId = categoryId;
    mBookmark = bookmarkId;
    mName = name;
    mIcon = getIconInternal();
    getXY();
  }

  private native ParcelablePointD getXY(int c, long b);
  private native String getIcon(int c, long b);

  private native double getScale(int category, long bookmark);
  private native String encode2Ge0Url(int category, long bookmark, boolean addName);

  private native void setBookmarkParams(int c, long b, String name, String type, String descr);
  private native int changeCategory(int oldCat, int newCat, long bmk);

  private native String getBookmarkDescription(int categoryId, long bookmark);

  @Override
  public double getScale()
  {
    return getScale(mCategoryId, mBookmark);
  }

  private void getXY()
  {
    final ParcelablePointD ll = getXY(mCategoryId, mBookmark);
    mMerX = ll.x;
    mMerY = ll.y;

    final double yRad = ll.y*Math.PI/180.0;
    final double lat = (180.0/Math.PI)*(2.0 * Math.atan(Math.exp(yRad)) - Math.PI/2.0);
    mLat = lat;
    mLon = ll.x;
  }

  public DistanceAndAzimut getDistanceAndAzimut(double cLat, double cLon, double north)
  {
    return Framework.getDistanceAndAzimut(mMerX, mMerY, cLat, cLon, north);
  }

  @Override
  public double getLat() { return mLat; }

  @Override
  public double getLon() { return mLon; }

  private Icon getIconInternal()
  {
    return BookmarkManager.getBookmarkManager()
        .getIconByName((mCategoryId >= 0) ? getIcon(mCategoryId, mBookmark) : "");
  }

  public Icon getIcon()
  {
    return mIcon;
  }

  @Override
  public String getName()
  {
    return mName;
  }

  public String getCategoryName(Context context)
  {
    if (mCategoryId >= 0)
    {
      return BookmarkManager.getBookmarkManager().getCategoryById(mCategoryId).getName();
    }
    else
    {
      mCategoryId = 0;
      return context.getString(R.string.my_places);
    }
  }

  public void setCategoryId(int catId)
  {
    if (catId != mCategoryId)
    {
      mBookmark = changeCategory(mCategoryId, catId, mBookmark);
      mCategoryId = catId;
    }
  }

  public void setParams(String name, Icon icon, String descr)
  {
    if (icon == null)
      icon = mIcon;

    if (!name.equals(getName()) || icon != mIcon || !descr.equals(getBookmarkDescription()))
    {
      setBookmarkParams(mCategoryId, mBookmark, name, icon.getType(), descr);
      mName = name;
    }
  }

  public int getCategoryId()
  {
    return mCategoryId;
  }

  public int getBookmarkId()
  {
    return mBookmark;
  }

  public String getBookmarkDescription()
  {
    return getBookmarkDescription(mCategoryId, mBookmark);
  }

  public String getGe0Url(boolean addName)
  {
    return encode2Ge0Url(mCategoryId, mBookmark, addName);
  }

  public String getHttpGe0Url(boolean addName)
  {
    String url = getGe0Url(addName);
    url = url.replaceFirst("ge0://", "http://ge0.me/");
    return url;
  }

  @Override
  public MapObjectType getType()
  {
    return MapObjectType.BOOKMARK;
  }
}
