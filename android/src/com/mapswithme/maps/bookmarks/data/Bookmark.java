package com.mapswithme.maps.bookmarks.data;

import android.content.Context;
import android.os.Parcel;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.R;
import com.mapswithme.util.Constants;

public class Bookmark extends MapObject
{
  private final Icon mIcon;
  private int mCategoryId;
  private int mBookmarkId;
  private double mMerX;
  private double mMerY;

  Bookmark(int categoryId, int bookmarkId, String name)
  {
    super(name, 0, 0, "");

    mCategoryId = categoryId;
    mBookmarkId = bookmarkId;
    mName = name;
    mIcon = getIconInternal();
    getXY();
  }

  @Override
  public void writeToParcel(Parcel dest, int flags)
  {
    dest.writeString(getType().toString());
    dest.writeInt(mCategoryId);
    dest.writeInt(mBookmarkId);
    dest.writeString(mName);
  }

  protected Bookmark(Parcel source)
  {
    this(source.readInt(), source.readInt(), source.readString());
  }

  private native ParcelablePointD getXY(int catId, long bookmarkId);

  private native String getIcon(int catId, long bookmarkId);

  private native double getScale(int catId, long bookmarkId);

  private native String encode2Ge0Url(int catId, long bookmarkId, boolean addName);

  private native void setBookmarkParams(int catId, long bookmarkId, String name, String type, String descr);

  private native int changeCategory(int oldCatId, int newCatId, long bookmarkId);

  private native String getBookmarkDescription(int categoryId, long bookmarkId);

  @Override
  public double getScale()
  {
    return getScale(mCategoryId, mBookmarkId);
  }

  private void getXY()
  {
    final ParcelablePointD ll = getXY(mCategoryId, mBookmarkId);
    mMerX = ll.x;
    mMerY = ll.y;

    mLat = Math.toDegrees(2.0 * Math.atan(Math.exp(Math.toRadians(ll.y))) - Math.PI / 2.0);
    mLon = ll.x;
  }

  public DistanceAndAzimut getDistanceAndAzimuth(double cLat, double cLon, double north)
  {
    return Framework.nativeGetDistanceAndAzimut(mMerX, mMerY, cLat, cLon, north);
  }

  @Override
  public double getLat() { return mLat; }

  @Override
  public double getLon() { return mLon; }

  private Icon getIconInternal()
  {
    return BookmarkManager.getIconByType((mCategoryId >= 0) ? getIcon(mCategoryId, mBookmarkId) : "");
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
      return BookmarkManager.INSTANCE.getCategoryById(mCategoryId).getName();
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
      mBookmarkId = changeCategory(mCategoryId, catId, mBookmarkId);
      mCategoryId = catId;
    }
  }

  public void setParams(String name, Icon icon, String descr)
  {
    if (icon == null)
      icon = mIcon;

    if (!name.equals(getName()) || icon != mIcon || !descr.equals(getBookmarkDescription()))
    {
      setBookmarkParams(mCategoryId, mBookmarkId, name, icon.getType(), descr);
      mName = name;
    }
  }

  public int getCategoryId()
  {
    return mCategoryId;
  }

  public int getBookmarkId()
  {
    return mBookmarkId;
  }

  public String getBookmarkDescription()
  {
    return getBookmarkDescription(mCategoryId, mBookmarkId);
  }

  public String getGe0Url(boolean addName)
  {
    return encode2Ge0Url(mCategoryId, mBookmarkId, addName);
  }

  public String getHttpGe0Url(boolean addName)
  {
    return getGe0Url(addName).replaceFirst(Constants.Url.GE0_PREFIX, Constants.Url.HTTP_GE0_PREFIX);
  }

  @Override
  public MapObjectType getType()
  {
    return MapObjectType.BOOKMARK;
  }

  @Override
  public String getPoiTypeName()
  {
    return BookmarkManager.INSTANCE.getCategoryById(mCategoryId).getName();
  }
}
