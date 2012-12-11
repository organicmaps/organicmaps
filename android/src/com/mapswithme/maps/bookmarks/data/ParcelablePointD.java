package com.mapswithme.maps.bookmarks.data;

import android.os.Parcel;
import android.os.Parcelable;

public class ParcelablePointD implements Parcelable
{
  public double x;
  public double y;

  @Override
  public int describeContents()
  {
    // TODO Auto-generated method stub
    return 0;
  }

  @Override
  public void writeToParcel(Parcel dest, int flags)
  {
    dest.writeDouble(x);
    dest.writeDouble(y);
  }

  private ParcelablePointD(Parcel in)
  {
    x = in.readDouble();
    y = in.readDouble();
  }

  public ParcelablePointD(double x, double y)
  {
    this.x = x;
    this.y = y;
  }

  public static final Parcelable.Creator<ParcelablePointD> CREATOR = new Parcelable.Creator<ParcelablePointD>()
  {
    public ParcelablePointD createFromParcel(Parcel in)
    {
      return new ParcelablePointD(in);
    }

    public ParcelablePointD[] newArray(int size)
    {
      return new ParcelablePointD[size];
    }
  };

}
