package com.mapswithme.maps.bookmarks.data;

import android.os.Parcel;
import android.os.Parcelable;
import android.support.annotation.Nullable;

public final class Banner implements Parcelable
{
  public static final Banner EMPTY = new Banner((String) null);

  public static final Creator<Banner> CREATOR = new Creator<Banner>()
  {
    @Override
    public Banner createFromParcel(Parcel in)
    {
      return new Banner(in);
    }

    @Override
    public Banner[] newArray(int size)
    {
      return new Banner[size];
    }
  };

  @Nullable
  private final String mId;

  public Banner(@Nullable String id)
  {
    mId = id;
  }

  protected Banner(Parcel in)
  {
    mId = in.readString();
  }

  @Nullable
  public String getId()
  {
    return mId;
  }

  @Override
  public int describeContents()
  {
    return 0;
  }

  @Override
  public void writeToParcel(Parcel dest, int flags)
  {
    dest.writeString(mId);
  }

  @Override
  public String toString()
  {
    return "Banner{" +
           "mId='" + mId + '\'' +
           '}';
  }
}
