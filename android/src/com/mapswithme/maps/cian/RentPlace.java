package com.mapswithme.maps.cian;

import android.os.Parcel;
import android.os.Parcelable;
import android.support.annotation.NonNull;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class RentPlace implements Parcelable
{
  private final double mLat;
  private final double mLon;
  @NonNull
  private final List<RentOffer> mOffers;

  public static final Creator<RentPlace> CREATOR = new Creator<RentPlace>()
  {
    @Override
    public RentPlace createFromParcel(Parcel in)
    {
      return new RentPlace(in);
    }

    @Override
    public RentPlace[] newArray(int size)
    {
      return new RentPlace[size];
    }
  };

  public RentPlace(double lat, double lon, @NonNull RentOffer[] offers)
  {
    mLat = lat;
    mLon = lon;
    mOffers = new ArrayList<>(Arrays.asList(offers));
  }

  private RentPlace(Parcel in)
  {
    mLat = in.readDouble();
    mLon = in.readDouble();
    mOffers = in.createTypedArrayList(RentOffer.CREATOR);
  }

  @Override
  public int describeContents()
  {
    return 0;
  }

  @Override
  public void writeToParcel(Parcel dest, int flags)
  {
    dest.writeDouble(mLat);
    dest.writeDouble(mLon);
    dest.writeTypedList(mOffers);
  }

  public double getLat()
  {
    return mLat;
  }

  public double getLon()
  {
    return mLon;
  }

  @NonNull
  public List<RentOffer> getOffers()
  {
    return mOffers;
  }

  @Override
  public boolean equals(Object o)
  {
    if (this == o) return true;
    if (o == null || getClass() != o.getClass()) return false;

    RentPlace rentPlace = (RentPlace) o;

    if (Double.compare(rentPlace.mLat, mLat) != 0) return false;
    if (Double.compare(rentPlace.mLon, mLon) != 0) return false;
    return mOffers.equals(rentPlace.mOffers);
  }

  @Override
  public int hashCode()
  {
    int result;
    long temp;
    temp = Double.doubleToLongBits(mLat);
    result = (int) (temp ^ (temp >>> 32));
    temp = Double.doubleToLongBits(mLon);
    result = 31 * result + (int) (temp ^ (temp >>> 32));
    result = 31 * result + mOffers.hashCode();
    return result;
  }
}
