package com.mapswithme.maps.bookmarks.data;

import android.content.Context;
import android.graphics.Point;

public class Bookmark
{
  private Icon mIcon;
  private Context mContext;
  private Point mPosition;
  private int mCategoryId;
  private long mBookmark;
  private double mLat = Double.NaN;
  private double mLon = Double.NaN;

  public Bookmark(Context context, Point position)
  {
    mContext = context.getApplicationContext();
    mPosition = position;
    getLatLon();
  }

  public Bookmark(Context context, int c, long b)
  {
    mContext = context.getApplicationContext();
    mCategoryId = c;
    mBookmark = b;
    mIcon = BookmarkManager.getPinManager(mContext).getIconByName(nGetIcon(c, b));
    getLatLon();
  }

  private native double[] nGetLatLon(int c, long b);
  private native String nGetNamePos(int x, int y);
  private native String nGetName(int c, long b);
  private native String nGetIconPos(int x, int y);
  private native String nGetIcon(int c, long b);
  private native void nChangeBookamark(double lat, double lon, String category, String name, String type);

  void getLatLon()
  {
    double [] ll = nGetLatLon(mCategoryId, mBookmark);
    mLat = ll[0];
    mLon = ll[1];
  }

  public double getLat()
  {
    return mLat;
  }

  public double getLon()
  {
    return mLon;
  }

  public Icon getIcon()
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
    return BookmarkManager.getPinManager(mContext).getIconByName(type);
  }

  public String getName(){
    String name = null;
    if (mPosition != null)
    {
      name = nGetNamePos(mPosition.x, mPosition.y);
    }
    else if(mLat != Double.NaN && mLon != Double.NaN)
    {
      name = nGetName(mCategoryId, mBookmark);
    }
    if (name == null)
    {
      throw new NullPointerException("Congratulations! You find a third way to specify bookmark!");
    }
    return name;
  }

  public String getCategoryName()
  {
    return BookmarkManager.getPinManager(mContext).getCategoryById(mCategoryId).getName();
  }
/*
  public void setCategory(String category)
  {

  }
  */
  public void setIcon(Icon icon)
  {
    mIcon = icon;
    nChangeBookamark(mLat, mLon,
                     getCategoryName(),
                     getName(), icon.getType());
  }

  public void setName(String name)
  {
    nChangeBookamark(mLat, mLon,
                     getCategoryName(),
                     name, mIcon.getType());
  }



  public void setCategory(String category, int catId)
  {
    nChangeBookamark(mLat, mLon,
                     category,
                     getName(), mIcon.getType());
    mCategoryId = catId;
  }

}
