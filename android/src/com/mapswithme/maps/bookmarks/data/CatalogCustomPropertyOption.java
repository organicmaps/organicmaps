package com.mapswithme.maps.bookmarks.data;

import android.os.Parcel;
import android.os.Parcelable;
import android.support.annotation.NonNull;

public class CatalogCustomPropertyOption implements Parcelable
{
  @NonNull
  private final String mValue;

  @NonNull
  private final String mLocalizedName;

  public CatalogCustomPropertyOption(@NonNull String value, @NonNull String localizedName)
  {
    mValue = value;
    mLocalizedName = localizedName;
  }

  protected CatalogCustomPropertyOption(Parcel in)
  {
    mValue = in.readString();
    mLocalizedName = in.readString();
  }

  @Override
  public void writeToParcel(Parcel dest, int flags)
  {
    dest.writeString(mValue);
    dest.writeString(mLocalizedName);
  }

  @Override
  public int describeContents()
  {
    return 0;
  }

  public static final Creator<CatalogCustomPropertyOption> CREATOR = new
      Creator<CatalogCustomPropertyOption>()
  {
    @Override
    public CatalogCustomPropertyOption createFromParcel(Parcel in)
    {
      return new CatalogCustomPropertyOption(in);
    }

    @Override
    public CatalogCustomPropertyOption[] newArray(int size)
    {
      return new CatalogCustomPropertyOption[size];
    }
  };

  @NonNull
  public String getValue() { return mValue; }

  @NonNull
  public String getLocalizedName() { return mLocalizedName; }
}
