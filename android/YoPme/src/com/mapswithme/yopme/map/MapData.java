package com.mapswithme.yopme.map;

import com.mapswithme.maps.api.MWMPoint;

import android.graphics.Bitmap;

public class MapData
{
  private final Bitmap mBitmap;
  private final MWMPoint mPoint;

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
