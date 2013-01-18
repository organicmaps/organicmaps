package com.mapswithme.maps.bookmarks.data;

import android.graphics.Point;
import android.os.Parcel;
import android.os.Parcelable;

public class ParcelPoint implements Parcelable
{
  private Point mPoint;
  public ParcelPoint(int x, int y)
  {
    mPoint = new Point(x, y);
  }

  private ParcelPoint(Parcel src)
  {
    this(src.readInt(), src.readInt());
  }

  public Point getPoint()
  {
    return mPoint;
  }

  public int getX()
  {
    return mPoint.x;
  }

  public int getY()
  {
    return mPoint.y;
  }

  public void setX(int x)
  {
    mPoint.x = x;
  }

  public void setY(int y)
  {
    mPoint.y = y;
  }

  @Override
  public int describeContents()
  {
    // TODO Auto-generated method stub
    return 0;
  }

  @Override
  public void writeToParcel(Parcel arg0, int arg1)
  {
    arg0.writeInt(mPoint.x);
    arg0.writeInt(mPoint.y);
  }

  public static final Parcelable.Creator<ParcelPoint> CREATOR
    = new Creator<ParcelPoint>()
    {

      @Override
      public ParcelPoint[] newArray(int size)
      {
        return new ParcelPoint[size];
      }

      @Override
      public ParcelPoint createFromParcel(Parcel source)
      {
        return new ParcelPoint(source);
      }
    };
}
