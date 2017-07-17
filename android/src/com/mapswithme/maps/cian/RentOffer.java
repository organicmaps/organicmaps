package com.mapswithme.maps.cian;

import android.os.Parcel;
import android.os.Parcelable;
import android.support.annotation.NonNull;

public final class RentOffer implements Parcelable
{
  @NonNull
  private final String mFlatType;
  private final int mRoomsCount;
  private final int mPrice;
  private final int mFloorNumber;
  private final int mFloorsCount;
  @NonNull
  private final String mUrl;
  @NonNull
  private final String mAddress;

  public static final Creator<RentOffer> CREATOR = new Creator<RentOffer>()
  {
    @Override
    public RentOffer createFromParcel(Parcel in)
    {
      return new RentOffer(in);
    }

    @Override
    public RentOffer[] newArray(int size)
    {
      return new RentOffer[size];
    }
  };

  public RentOffer(@NonNull String flatType, int roomsCount, int price, int floorNumber,
                   int floorsCount, @NonNull String url, @NonNull String address)
  {
    mFlatType = flatType;
    mRoomsCount = roomsCount;
    mPrice = price;
    mFloorNumber = floorNumber;
    mFloorsCount = floorsCount;
    mUrl = url;
    mAddress = address;
  }

  private RentOffer(Parcel in)
  {
    mFlatType = in.readString();
    mRoomsCount = in.readInt();
    mPrice = in.readInt();
    mFloorNumber = in.readInt();
    mFloorsCount = in.readInt();
    mUrl = in.readString();
    mAddress = in.readString();
  }

  @Override
  public int describeContents()
  {
    return 0;
  }

  @Override
  public void writeToParcel(Parcel dest, int flags)
  {
    dest.writeString(mFlatType);
    dest.writeInt(mRoomsCount);
    dest.writeInt(mPrice);
    dest.writeInt(mFloorNumber);
    dest.writeInt(mFloorsCount);
    dest.writeString(mUrl);
    dest.writeString(mAddress);
  }

  @NonNull
  public String getFlatType()
  {
    return mFlatType;
  }

  public int getRoomsCount()
  {
    return mRoomsCount;
  }

  public int getPrice()
  {
    return mPrice;
  }

  public int getFloorNumber()
  {
    return mFloorNumber;
  }

  public int getFloorsCount()
  {
    return mFloorsCount;
  }

  @NonNull
  public String getUrl()
  {
    return mUrl;
  }

  @NonNull
  public String getAddress()
  {
    return mAddress;
  }

  @Override
  public boolean equals(Object o)
  {
    if (this == o) return true;
    if (o == null || getClass() != o.getClass()) return false;

    RentOffer rentOffer = (RentOffer) o;

    if (mRoomsCount != rentOffer.mRoomsCount) return false;
    if (mPrice != rentOffer.mPrice) return false;
    if (mFloorNumber != rentOffer.mFloorNumber) return false;
    if (mFloorsCount != rentOffer.mFloorsCount) return false;
    if (!mFlatType.equals(rentOffer.mFlatType)) return false;
    if (!mUrl.equals(rentOffer.mUrl)) return false;
    return mAddress.equals(rentOffer.mAddress);
  }

  @Override
  public int hashCode()
  {
    int result = mFlatType.hashCode();
    result = 31 * result + mRoomsCount;
    result = 31 * result + mPrice;
    result = 31 * result + mFloorNumber;
    result = 31 * result + mFloorsCount;
    result = 31 * result + mUrl.hashCode();
    result = 31 * result + mAddress.hashCode();
    return result;
  }
}
