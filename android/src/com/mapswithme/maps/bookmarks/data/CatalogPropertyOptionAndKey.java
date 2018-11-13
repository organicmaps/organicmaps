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

  protected CatalogPropertyOptionAndKey(Parcel in)
  {
    mPropertyKey = in.readString();
    mOption = in.readParcelable(CatalogCustomPropertyOption.class.getClassLoader());
  }

  @Override
  public void writeToParcel(Parcel dest, int flags)
  {
    dest.writeString(mPropertyKey);
    dest.writeParcelable(mOption, flags);
  }

  @Override
  public int describeContents()
  {
    return 0;
  }

  public static final Creator<CatalogPropertyOptionAndKey> CREATOR = new
      Creator<CatalogPropertyOptionAndKey>()
  {
    @Override
    public CatalogPropertyOptionAndKey createFromParcel(Parcel in)
    {
      return new CatalogPropertyOptionAndKey(in);
    }

    @Override
    public CatalogPropertyOptionAndKey[] newArray(int size)
    {
      return new CatalogPropertyOptionAndKey[size];
    }
  };

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
}
