package com.mapswithme.maps.bookmarks.data;

import android.content.Context;
import android.graphics.Point;

public class Bookmark
{
  private Icon mIcon;
  private Context mContext;
  private Point mPosition;
  private int mCategoryId = -1;
  private int mBookmark;
  private double mLat = Double.NaN;
  private double mLon = Double.NaN;
  private String m_previewString = "";
  private final boolean mIsPreviewBookmark;

  // For bookmark preview
  Bookmark(Context context, Point pos, String name)
  {
    mIsPreviewBookmark = true;
    mContext = context.getApplicationContext();
    mPosition = pos;
    m_previewString = name;
    getLatLon(mPosition);
  }

  Bookmark(Context context, Point position, int nextCat, int b)
  {
    mIsPreviewBookmark = false;
    mContext = context.getApplicationContext();
    mPosition = position;
    getLatLon(position);
    mBookmark = b;
    mIcon = getIconInternal();
    mCategoryId = nextCat;
    String name = getName();
    changeBookmark(getCategoryName(), name, mIcon.getType());
  }


  Bookmark(Context context, int c, int b)
  {
    mIsPreviewBookmark = false;
    mContext = context.getApplicationContext();
    mCategoryId = c;
    mBookmark = b;
    mIcon = getIconInternal();// BookmarkManager.getBookmarkManager(mContext).getIconByName(nGetIcon(c, b));
    getLatLon();
  }

  private void getLatLon(Point position)
  {
    ParcelablePointD ll = nPtoG(position.x, position.y);
    mLat = ll.x;
    mLon = ll.y;
  }

  private native DistanceAndAthimuth nGetDistanceAndAzimuth(double lat, double lon, double cLat, double cLon, double north);
  private native Point nGtoP(double lat, double lon);
  private native ParcelablePointD nPtoG(int x, int y);
  private native ParcelablePointD nGetLatLon(int c, long b);
  private native String nGetNamePos(int x, int y);
  private native String nGetName(int c, long b);
  private native String nGetIconPos(int x, int y);
  private native String nGetIcon(int c, long b);
  private native void nChangeBookmark(double lat, double lon, String category, String name, String type);
  private native String nGetBookmarkDescription(int categoryId, long bookmark);
  private native void nSetBookmarkDescription(int categoryId, long bookmark, String newDescr);
  private native String nGetBookmarkDescriptionPos(int categoryId, int bookmark);

  void getLatLon()
  {
    ParcelablePointD ll = nGetLatLon(mCategoryId, mBookmark);
    mLat = ll.x;
    mLon = ll.y;
    mPosition = nGtoP(mLat, mLon);
  }

  public DistanceAndAthimuth getDistanceAndAthimuth(double cLat, double cLon, double north)
  {
    return nGetDistanceAndAzimuth(mLat, mLon, cLat, cLon, north);
  }

  public Point getPosition()
  {
    return nGtoP(mLat, mLon);
  }

  public double getLat()
  {
    return mLat;
  }

  public double getLon()
  {
    return mLon;
  }

  private Icon getIconInternal()
  {
    if (mCategoryId > -1)
    {
      String type = null;
      if (mPosition != null)
      {
        type = nGetIconPos(mPosition.x, mPosition.y);
      }
      else if(mLat != Double.NaN && mLon != Double.NaN)
      {
        type = nGetIcon(mCategoryId, mBookmark);
      }
      if (type == null)
      {
        throw new NullPointerException("Congratulations! You find a third way to specify bookmark!");
      }
      return BookmarkManager.getBookmarkManager(mContext).getIconByName(type);
    }
    else
    {
      return BookmarkManager.getBookmarkManager(mContext).getIconByName("");
    }

  }

  public Icon getIcon()
  {
    return mIcon;
  }

  public String getName(){
    if (mCategoryId > -1 && BookmarkManager.getBookmarkManager(mContext).getCategoryById(mCategoryId).getSize() > mBookmark)
    {
      String name = null;
      if(mLat != Double.NaN && mLon != Double.NaN)
      {
        name = nGetName(mCategoryId, mBookmark);
      }
      if (name == null)
      {
        throw new NullPointerException("Congratulations! You find a third way to specify bookmark!");
      }
      return name;
    }
    else
    {
      return m_previewString;
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
      //TODO change string resources
      mCategoryId++;
      return "My Places";
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
    mCategoryId = catId;
    mBookmark = BookmarkManager.getBookmarkManager(mContext).getCategoryById(mCategoryId).getSize() - 1;
  }

  private void changeBookmark(String category, String name, String type)
  {
    nChangeBookmark(mLat, mLon, category, name, type);
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
    if (mCategoryId > -1)
    {
      String name = null;
      if(mLat != Double.NaN && mLon != Double.NaN)
      {
        name = nGetBookmarkDescription(mCategoryId, mBookmark);
      }
      if (name == null)
      {
        throw new NullPointerException("Congratulations! You find a third way to specify bookmark!");
      }
      return name;
    }
    else
    {
      return m_previewString;
    }
  }

  public void setDescription(String n)
  {
    nSetBookmarkDescription(mCategoryId, mBookmark, n);
  }

  //TODO stub
  public boolean isPreviewBookmark()
  {
    return mIsPreviewBookmark;
  }
}
