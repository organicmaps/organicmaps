package com.mapswithme.yopme.map;

import java.io.Serializable;

import com.mapswithme.yopme.PoiPoint;

import android.graphics.Bitmap;

public class MapData implements Serializable
{
  private static final long serialVersionUID = 1L;

  private final Bitmap mBitmap;
  private final PoiPoint mPoint;

  public MapData()
  {
    mBitmap = null;
    mPoint = new PoiPoint(0, 0, "Unknown");
  }

  public MapData(Bitmap bitmap, PoiPoint point)
  {
    this.mBitmap = bitmap;
    this.mPoint = point;
  }

  public PoiPoint getPoint()
  {
    return mPoint;
  }

  public Bitmap getBitmap()
  {
    return mBitmap;
  }
}
