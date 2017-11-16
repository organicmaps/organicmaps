package com.mapswithme.maps.search;

import android.support.annotation.NonNull;

class BookingFilterParams
{
  static class Room
  {
    // This value is corresponds to AvailabilityParams::Room::kNoChildren in core.
    static final int NO_CHILDREN = -1;

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
  }

  private long mCheckinMillisec;
  private long  mCheckoutMillisec;
  @NonNull
  private Room[] mRooms;

  BookingFilterParams(long checkinMillisec, long checkoutMillisec, @NonNull Room[] rooms)
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
