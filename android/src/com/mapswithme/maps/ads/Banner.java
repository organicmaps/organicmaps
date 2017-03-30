package com.mapswithme.maps.ads;

import android.os.Parcel;
import android.os.Parcelable;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

public final class Banner implements Parcelable
{
  public static final Banner EMPTY = new Banner((String) null);
  private static final int TYPE_FACEBOOK = 1;
  private static final int TYPE_RB = 2;

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
  //TODO: pass as argument constructor and read a correct value
  private int mType = TYPE_FACEBOOK;
  @Nullable
  private BannerKey mKey;

  public Banner(@Nullable String id)
  {
    mId = id;
  }

  public Banner(@NonNull String id, int type)
  {
    this(id);
    mType = type;
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

  @NonNull
  BannerKey getKey()
  {
    if (mKey != null)
      return mKey;

    String provider = getProvider();
    //noinspection ConstantConditions
    mKey = new BannerKey(provider, mId);

    return mKey;
  }

  @NonNull
  String getProvider()
  {
    switch (mType)
    {
      case TYPE_FACEBOOK:
        return Providers.FACEBOOK;
      case TYPE_RB:
        return Providers.MY_TARGET;
      default:
        throw new AssertionError("Unsupported banner type: " + mType);
    }
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

  @Override
  public boolean equals(Object o)
  {
    if (this == o) return true;
    if (o == null || getClass() != o.getClass()) return false;

    Banner banner = (Banner) o;

    return mId != null ? mId.equals(banner.mId) : banner.mId == null;
  }

  @Override
  public int hashCode()
  {
    return mId != null ? mId.hashCode() : 0;
  }
}
