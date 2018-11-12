package com.mapswithme.maps.bookmarks.data;

import android.os.Parcel;
import android.os.Parcelable;
import android.support.annotation.NonNull;

public class CatalogPropertyOptionAndKey implements Parcelable
{
  @NonNull
  private final String mPropertyKey;

  @NonNull
  private final CatalogCustomPropertyOption mOption;

  public CatalogPropertyOptionAndKey(@NonNull String propertyKey,
                                     @NonNull CatalogCustomPropertyOption option)
  {
    mPropertyKey = propertyKey;
    mOption = option;
  }

  @Override
  public int describeContents()
  {
    return 0;
  }

  @NonNull
  public String getKey()
  {
    return mPropertyKey;
  }

  @NonNull
  public CatalogCustomPropertyOption getOption()
  {
    return mOption;
  }

  @Override
  public void writeToParcel(Parcel dest, int flags)
  {
    dest.writeString(this.mPropertyKey);
    dest.writeParcelable(this.mOption, flags);
  }

  protected CatalogPropertyOptionAndKey(Parcel in)
  {
    this.mPropertyKey = in.readString();
    this.mOption = in.readParcelable(CatalogCustomPropertyOption.class.getClassLoader());
  }

  public static final Creator<CatalogPropertyOptionAndKey> CREATOR = new Creator<CatalogPropertyOptionAndKey>()
  {
    @Override
    public CatalogPropertyOptionAndKey createFromParcel(Parcel source)
    {
      return new CatalogPropertyOptionAndKey(source);
    }

    @Override
    public CatalogPropertyOptionAndKey[] newArray(int size)
    {
      return new CatalogPropertyOptionAndKey[size];
    }
  };
}
