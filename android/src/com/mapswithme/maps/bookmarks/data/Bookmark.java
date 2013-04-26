package com.mapswithme.maps.bookmarks.data;

import android.content.Context;

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


  Bookmark(Context context, int c, int b)
  {
    mContext = context.getApplicationContext();
    mCategoryId = c;
    mBookmark = b;
    mIcon = getIconInternal();
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
  private native String getName(int c, long b);
  private native String getIcon(int c, long b);

  private native double getScale(int category, long bookmark);
  private native String encode2Ge0Url(int category, long bookmark);

  private native void setBookmarkParams(int c, long b, String name, String type, String descr);
  private native int changeCategory(int oldCat, int newCat, long bmk);

  private native String getBookmarkDescription(int categoryId, long bookmark);

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
      setBookmarkParams(mCategoryId, mBookmark, name, icon.getType(), descr);
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

  public String getGe0Url()
  {
    return encode2Ge0Url(mCategoryId, mBookmark);
  }
  
  public String getHttpGe0Url() 
  {
    String url = getGe0Url();
    url = url.replaceFirst("ge0://", "http://ge0.me/");
    return url;
  }
}
