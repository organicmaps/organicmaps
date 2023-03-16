package app.organicmaps.bookmarks.data;

import android.graphics.Point;
import android.os.Parcel;
import android.os.Parcelable;

// TODO consider removal and usage of platform PointF
public class ParcelablePointD implements Parcelable
{
  public double x;
  public double y;

  public int getRoundX()
  {
    return (int) Math.round(x);
  }

  public int getRoundY()
  {
    return (int) Math.round(y);
  }

  public Point getRoundedPoint()
  {
    return new Point(getRoundX(), getRoundY());
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
