package com.mapswithme.maps.bookmarks.data;

import android.content.Context;
import android.graphics.Point;

public class Bookmark
{
  private Icon mIcon;
  private Context mContext;
  private Point mPosition;
  private String mCategoryName;
  private long mBookmark;
  private double mLat = Double.NaN;
  private double mLon = Double.NaN;

  public Bookmark(Context context, Point position)
  {
    mContext = context.getApplicationContext();
    mPosition = position;
  }

  public Bookmark(Context context, String c, long b)
  {
    mContext = context.getApplicationContext();
    mCategoryName = c;
    mBookmark = b;
  }

  private native double[] nGetLatLon(String c, long b);
  private native String nGetNamePos(int x, int y);
  private native String nGetName(String c, long b);
  private native String nGetIconPos(int x, int y);
  private native String nGetIcon(String c, long b);
  private native void nChangeBookamark(double lat, double lon, String category, String name, String type);

  void getLatLon()
  {
    double [] ll = nGetLatLon(mCategoryName, mBookmark);
    mLat = ll[0];
    mLon = ll[1];
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
      type = nGetIcon(mCategoryName, mBookmark);
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
      name = nGetName(mCategoryName, mBookmark);
    }
    if (name == null)
    {
      throw new NullPointerException("Congratulations! You find a third way to specify bookmark!");
    }
    return name;
  }

  public void changeBookmark(String category, String name, Icon icon)
  {
    nChangeBookamark(mLat, mLon, category, name, icon.getType());
  }
}
