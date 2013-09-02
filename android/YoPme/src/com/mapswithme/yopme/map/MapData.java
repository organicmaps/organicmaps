package com.mapswithme.yopme.map;

import java.io.Serializable;

import com.mapswithme.maps.api.MWMPoint;

import android.graphics.Bitmap;

public class MapData implements Serializable
{
  private static final long serialVersionUID = 1L;

  private final Bitmap mBitmap;
  private final MWMPoint mPoint;

  public MapData()
  {
    mBitmap = null;
    mPoint = new MWMPoint(0, 0, "Unknown");
  }

  public MapData(Bitmap bitmap, MWMPoint point)
  {
    this.mBitmap = bitmap;
    this.mPoint = point;
  }

  public MWMPoint getPoint()
  {
    return mPoint;
  }

  public Bitmap getBitmap()
  {
    return mBitmap;
  }
}
