package com.mapswithme.maps.bookmarks.data;

import android.content.Context;

import com.mapswithme.maps.MapObjectFragment.MapObjectType;
import com.mapswithme.maps.Framework;
import com.mapswithme.maps.R;

public class Bookmark extends MapObject
{
  private Icon mIcon;
  private Context mContext;
  private ParcelablePointD mPosition;
  private int mCategoryId = -1;
  private int mBookmark;
  private double mMerX = Double.NaN;
  private double mMerY = Double.NaN;
  private double mLon = Double.NaN;
  private double mLat = Double.NaN;


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
    mMerX = ll.x;
    mMerY = ll.y;
  }

  public static ParcelablePointD GtoP(ParcelablePointD p)
  {
    return g2p(p.x, p.y);
  }

  public static ParcelablePointD PtoG(ParcelablePointD p)
  {
    return p2g(p.x, p.y);
  }

  private static native ParcelablePointD g2p(double x, double y);
  private static native ParcelablePointD p2g(double px, double py);
  private native ParcelablePointD getXY(int c, long b);
  private native String getName(int c, long b);
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

  void getXY()
  {
    ParcelablePointD ll = getXY(mCategoryId, mBookmark);
    mMerX = ll.x;
    mMerY = ll.y;

    final double yRad = ll.y*Math.PI/180.0;
    final double lat = (180.0/Math.PI)*(2.0 * Math.atan(Math.exp(yRad)) - Math.PI/2.0);
    mLat = lat;
    mLon = ll.x;

    mPosition = g2p(mMerX, mMerY);
  }

  public DistanceAndAzimut getDistanceAndAzimut(double cLat, double cLon, double north)
  {
    return Framework.getDistanceAndAzimut(mMerX, mMerY, cLat, cLon, north);
  }

  public ParcelablePointD getPosition()
  {
    return g2p(mMerX, mMerY);
  }

  @Override
  public double getLat() { return mLat; }

  @Override
  public double getLon() { return mLon; }

  private Icon getIconInternal()
  {
    return BookmarkManager.getBookmarkManager(mContext).getIconByName((mCategoryId >= 0) ? getIcon(mCategoryId, mBookmark) : "");
  }

  public Icon getIcon()
  {
    return mIcon;
  }

  @Override
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
