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

  @NonNull
  public String getValue() { return mValue; }

  @NonNull
  public String getLocalizedName() { return mLocalizedName; }

  @Override
  public int describeContents()
  {
    return 0;
  }

  @Override
  public void writeToParcel(Parcel dest, int flags)
  {
    dest.writeString(this.mValue);
    dest.writeString(this.mLocalizedName);
  }

  protected CatalogCustomPropertyOption(Parcel in)
  {
    this.mValue = in.readString();
    this.mLocalizedName = in.readString();
  }

  public static final Creator<CatalogCustomPropertyOption> CREATOR = new Creator<CatalogCustomPropertyOption>()
  {
    @Override
    public CatalogCustomPropertyOption createFromParcel(Parcel source)
    {
      return new CatalogCustomPropertyOption(source);
    }

    @Override
    public CatalogCustomPropertyOption[] newArray(int size)
    {
      return new CatalogCustomPropertyOption[size];
    }
  };
}
