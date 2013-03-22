package com.mapswithme.maps.bookmarks.data;

import android.content.Context;
import android.graphics.Point;
import android.util.Log;

import com.mapswithme.maps.R;

public class Bookmark
{
  private Icon mIcon;
  private Context mContext;
  private ParcelablePointD mPosition;
  private int mCategoryId = -1;
  private int mBookmark;
  private double mMercatorX = Double.NaN;
  private double mMercatorY = Double.NaN;

  //private String mPreviewString = "";
  //private final boolean mIsPreviewBookmark;

  /*
  // For bookmark preview
  Bookmark(Context context, ParcelablePointD pos, String name)
  {
    mIsPreviewBookmark = true;
    mContext = context.getApplicationContext();
    mPosition = pos;
    mPreviewString = name;
    getXY(mPosition);
  }
   */

  Bookmark(Context context, ParcelablePointD position, int nextCat, int b)
  {
    //mIsPreviewBookmark = false;
    mContext = context.getApplicationContext();
    mPosition = position;
    getXY(position);
    mBookmark = b;
    mIcon = getIconInternal();
    String name = getName();
    mCategoryId = nextCat;
    changeBookmark(getCategoryName(), name, mIcon.getType());
    Point bookmark = BookmarkManager.getBookmark(position.x, position.y);
    mBookmark = bookmark.y;
    Log.d("Bookmark indices", " " + mCategoryId+ " "+ mBookmark);
  }


  Bookmark(Context context, int c, int b)
  {
    //mIsPreviewBookmark = false;
    mContext = context.getApplicationContext();
    mCategoryId = c;
    mBookmark = b;
    mIcon = getIconInternal();// BookmarkManager.getBookmarkManager(mContext).getIconByName(nGetIcon(c, b));
    getXY();
  }

  private void getXY(ParcelablePointD position)
  {
    ParcelablePointD ll = p2g(position.x, position.y);
    mMercatorX = ll.x;
    mMercatorY = ll.y;
  }

  public static ParcelablePointD GtoP(ParcelablePointD p)
  {
    return g2p(p.x, p.y);
  }

  public static ParcelablePointD PtoG(ParcelablePointD p)
  {
    return p2g(p.x, p.y);
  }

  private native DistanceAndAzimut getDistanceAndAzimut(double x, double y, double cLat, double cLon, double north);
  private static native ParcelablePointD g2p(double x, double y);
  private static native ParcelablePointD p2g(double px, double py);
  private native ParcelablePointD getXY(int c, long b);
  private native String getNamePos(double px, double py);
  private native String getName(int c, long b);
  private native String getIconPos(double px, double py);
  private native String getIcon(int c, long b);
  private native void changeBookmark(double x, double y, String category, String name, String type, String descr);
  private native String getBookmarkDescription(int categoryId, long bookmark);
  private native void setBookmarkDescription(int categoryId, long bookmark, String newDescr);
  private native String getBookmarkDescriptionPos(int categoryId, int bookmark);

  void getXY()
  {
    ParcelablePointD ll = getXY(mCategoryId, mBookmark);
    mMercatorX = ll.x;
    mMercatorY = ll.y;
    mPosition = g2p(mMercatorX, mMercatorY);
  }

  public DistanceAndAzimut getDistanceAndAzimut(double cLat, double cLon, double north)
  {
    return getDistanceAndAzimut(mMercatorX, mMercatorY, cLat, cLon, north);
  }

  public ParcelablePointD getPosition()
  {
    return g2p(mMercatorX, mMercatorY);
  }

  public double getLat()
  {
    return mMercatorX;
  }

  public double getLon()
  {
    return mMercatorY;
  }

  private Icon getIconInternal()
  {
    return BookmarkManager.getBookmarkManager(mContext).getIconByName((mCategoryId >= 0) ? getIcon(mCategoryId, mBookmark) : "");
  }

  public Icon getIcon()
  {
    return mIcon;
  }

  public String getName()
  {
    if (mCategoryId >= 0 && BookmarkManager.getBookmarkManager(mContext).getCategoryById(mCategoryId).getSize() > mBookmark)
    {
      return getName(mCategoryId, mBookmark);
    }
    else
    {
      return "";
    }
  }

  public String getCategoryName()
  {
    if (mCategoryId >= 0)
    {
      return BookmarkManager.getBookmarkManager(mContext).getCategoryById(mCategoryId).getName();
    }
    else
    {
      mCategoryId = 0;
      return mContext.getString(R.string.my_places);
    }
  }

  public void setIcon(Icon icon)
  {
    mIcon = icon;
    changeBookmark(getCategoryName(), getName(), icon.getType());
  }

  public void setName(String name)
  {
    changeBookmark(getCategoryName(), name, mIcon.getType());
  }

  public void setCategoryId(int catId)
  {
    setCategory(BookmarkManager.getBookmarkManager(mContext).getCategoryById(catId).getName(), catId);
  }

  public void setCategory(String category, int catId)
  {
    changeBookmark(category, getName(), mIcon.getType());

    /// @todo This is not correct, but acceptable in current usage (object is not using later).
    mCategoryId = catId;
    mBookmark = BookmarkManager.getBookmarkManager(mContext).getCategoryById(mCategoryId).getSize() - 1;
  }

  public void setParams(String name, Icon icon, String descr)
  {
    if (icon == null)
      icon = mIcon;

    if (!name.equals(getName()) || icon != mIcon || !descr.equals(getBookmarkDescription()))
      changeBookmark(mMercatorX, mMercatorY, getCategoryName(), name, icon.getType(), descr);
  }

  private void changeBookmark(String category, String name, String type)
  {
    changeBookmark(mMercatorX, mMercatorY, category, name, type, null);
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
    //if (!mIsPreviewBookmark)
    //{
    return getBookmarkDescription(mCategoryId, mBookmark);
    //}
    //else
    //{
    //  return mPreviewString;
    //}
  }

  public void setDescription(String n)
  {
    setBookmarkDescription(mCategoryId, mBookmark, n);
  }
}
