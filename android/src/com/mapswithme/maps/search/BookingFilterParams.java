package com.mapswithme.maps.search;

import android.os.Parcel;
import android.os.Parcelable;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

public class BookingFilterParams implements Parcelable
{
  protected BookingFilterParams(Parcel in)
  {
    mCheckinMillisec = in.readLong();
    mCheckoutMillisec = in.readLong();
    mRooms = in.createTypedArray(Room.CREATOR);
  }

  public static final Creator<BookingFilterParams> CREATOR = new Creator<BookingFilterParams>()
  {
    @Override
    public BookingFilterParams createFromParcel(Parcel in)
    {
      return new BookingFilterParams(in);
    }

    @Override
    public BookingFilterParams[] newArray(int size)
    {
      return new BookingFilterParams[size];
    }
  };

  @Override
  public int describeContents()
  {
    return 0;
  }

  @Override
  public void writeToParcel(Parcel dest, int flags)
  {
    dest.writeLong(mCheckinMillisec);
    dest.writeLong(mCheckoutMillisec);
    dest.writeTypedArray(mRooms, flags);
  }

  static class Room implements Parcelable
  {
    // This value is corresponds to AvailabilityParams::Room::kNoChildren in core.
    static final int NO_CHILDREN = -1;
    static final Room DEFAULT = new Room(2);
    private int mAdultsCount;
    private int mAgeOfChild;

    Room(int adultsCount)
    {
      mAdultsCount = adultsCount;
      mAgeOfChild = NO_CHILDREN;
    }

    Room(int adultsCount, int ageOfChild)
    {
      mAdultsCount = adultsCount;
      mAgeOfChild = ageOfChild;
    }

    protected Room(Parcel in)
    {
      mAdultsCount = in.readInt();
      mAgeOfChild = in.readInt();
    }

    public static final Creator<Room> CREATOR = new Creator<Room>()
    {
      @Override
      public Room createFromParcel(Parcel in)
      {
        return new Room(in);
      }

      @Override
      public Room[] newArray(int size)
      {
        return new Room[size];
      }
    };

    @Override
    public int describeContents()
    {
      return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags)
    {
      dest.writeInt(mAdultsCount);
      dest.writeInt(mAgeOfChild);
    }
  }

  private long mCheckinMillisec;
  private long  mCheckoutMillisec;
  @NonNull
  private final Room[] mRooms;

  BookingFilterParams(long checkinMillisec, long checkoutMillisec, @NonNull Room... rooms)
  {
    mCheckinMillisec = checkinMillisec;
    mCheckoutMillisec = checkoutMillisec;
    mRooms = rooms;
  }

  public long getCheckinMillisec()
  {
    return mCheckinMillisec;
  }

  public long getCheckoutMillisec()
  {
    return mCheckoutMillisec;
  }

  @NonNull
  public Room[] getRooms()
  {
    return mRooms;
  }
}
