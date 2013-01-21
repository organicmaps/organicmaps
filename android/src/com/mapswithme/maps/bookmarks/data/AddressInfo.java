package com.mapswithme.maps.bookmarks.data;

import com.mapswithme.maps.R;
import com.mapswithme.util.Utils;

import android.content.Context;
import android.os.Parcel;
import android.os.Parcelable;
import android.text.TextUtils;

public class AddressInfo implements Parcelable
{
  private String mName;
  private String mType;
  private ParcelablePointD mPXPivot;

  public AddressInfo(String name, String type, double px, double py)
  {
    mName = name;
    mType = type;
    mPXPivot = new ParcelablePointD(px, py);
  }

  private AddressInfo(Parcel in)
  {
    mName = in.readString();
    mType = in.readString();
    mPXPivot = in.readParcelable(ParcelablePointD.class.getClassLoader());
  }

  public AddressInfo(String name, String amenity, ParcelablePointD mLocation)
  {
    this(name, amenity, mLocation.x, mLocation.y);
  }

  public ParcelablePointD getPosition()
  {
    return mPXPivot;
  }

  public String getBookmarkName(Context context)
  {
    if (TextUtils.isEmpty(mName) && TextUtils.isEmpty(mType))
    {
      return Utils.toTitleCase(context.getString(R.string.dropped_pin));
    }
    else if (TextUtils.isEmpty(mName) || TextUtils.isEmpty(mType))
    {
      return Utils.toTitleCase(TextUtils.isEmpty(mName) ? mType : mName);
    }
    else
      return String.format("%s (%s)", Utils.toTitleCase(mName), Utils.toTitleCase(mType));
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
    dest.writeString(mName);
    dest.writeString(mType);
    dest.writeParcelable(mPXPivot, 0);
  }

  public static final Parcelable.Creator<AddressInfo> CREATOR = new Parcelable.Creator<AddressInfo>()
  {
    public AddressInfo createFromParcel(Parcel in)
    {
      return new AddressInfo(in);
    }

    public AddressInfo[] newArray(int size)
    {
      return new AddressInfo[size];
    }
  };
}
