package com.mapswithme.maps.bookmarks.data;

import android.content.Context;
import android.graphics.Point;

public class Bookmark
{
  private Icon mIcon;
  private Context mContext;
  private Point mPosition;
  private int mCategoryId = -1;
  private long mBookmark;
  private double mLat = Double.NaN;
  private double mLon = Double.NaN;

  Bookmark(Context context, Point position, int nextCat)
  {
    mContext = context.getApplicationContext();
    mPosition = position;
    getLatLon(position);
    changeBookmark(getCategoryName(), getName(), getIcon().getType());
    if (nextCat == -1)
    {
      nextCat++;
    }
    mCategoryId = nextCat;
  }

  private void getLatLon(Point position)
  {
    double[] ll = nPtoG(position.x, position.y);
    mLat = ll[0];
    mLon = ll[1];
  }

  Bookmark(Context context, int c, long b)
  {
    mContext = context.getApplicationContext();
    mCategoryId = c;
    mBookmark = b;
    mIcon = BookmarkManager.getPinManager(mContext).getIconByName(nGetIcon(c, b));
    getLatLon();
  }

  private native double[] nPtoG(int x, int y);
  private native double[] nGetLatLon(int c, long b);
  private native String nGetNamePos(int x, int y);
  private native String nGetName(int c, long b);
  private native String nGetIconPos(int x, int y);
  private native String nGetIcon(int c, long b);
  private native void nChangeBookmark(double lat, double lon, String category, String name, String type);

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
      return BookmarkManager.getPinManager(mContext).getIconByName(type);
    }
    else
    {
      return BookmarkManager.getPinManager(mContext).getIconByName("");
    }

  }

  public String getName(){
    if (mCategoryId > -1)
    {
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
    else
    {
      return "Bookmark";
    }
  }

  public String getCategoryName()
  {
    if (mCategoryId >= 0)
    {
      return BookmarkManager.getPinManager(mContext).getCategoryById(mCategoryId).getName();
    }
    else
    {
      //TODO change string resources
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

  public void setCategory(String category, int catId)
  {
    changeBookmark(category, getName(), mIcon.getType());
    mCategoryId = catId;
  }

  private void changeBookmark(String category, String name, String type)
  {
    nChangeBookmark(mLat, mLon, category, name, type);
  }

  public int getCategoryId()
  {
    return mCategoryId;
  }
}
