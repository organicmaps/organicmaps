package com.mapswithme.maps.bookmarks.data;

import android.graphics.Point;
import android.os.Parcel;
import android.os.Parcelable;

public class ParcelablePoint implements Parcelable
{
  private Point mInternalPoint;

  public Point getPoint()
  {
    return mInternalPoint;
  }

  @Override
  public int describeContents()
  {
    // TODO Auto-generated method stub
    return 0;
  }

  @Override
  public void writeToParcel(Parcel dest, int flags)
  {
    dest.writeInt(mInternalPoint.x);
    dest.writeInt(mInternalPoint.y);
  }

  private ParcelablePoint(Parcel in)
  {
    mInternalPoint = new Point(in.readInt(), in.readInt());
  }

  public ParcelablePoint(int x, int y)
  {
    mInternalPoint = new Point(x, y);
  }

  public ParcelablePoint(Point position)
  {
    mInternalPoint = new Point(position);
  }

  public static final Parcelable.Creator<ParcelablePoint> CREATOR = new Parcelable.Creator<ParcelablePoint>()
  {
    public ParcelablePoint createFromParcel(Parcel in)
    {
      return new ParcelablePoint(in);
    }

    public ParcelablePoint[] newArray(int size)
    {
      return new ParcelablePoint[size];
    }
  };

}
