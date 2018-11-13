package com.mapswithme.maps.bookmarks.data;

import android.os.Parcel;
import android.os.Parcelable;
import android.support.annotation.NonNull;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

public class CatalogCustomProperty implements Parcelable
{
  @NonNull
  private final String mKey;

  @NonNull
  private final String mLocalizedName;

  private final boolean mRequired;

  @NonNull
  private final List<CatalogCustomPropertyOption> mOptions;

  public CatalogCustomProperty(@NonNull String key, @NonNull String localizedName,
                               boolean required, @NonNull CatalogCustomPropertyOption[] options)
  {
    mKey = key;
    mLocalizedName = localizedName;
    mRequired = required;
    mOptions = Collections.unmodifiableList(Arrays.asList(options));
  }

  protected CatalogCustomProperty(Parcel in)
  {
    mKey = in.readString();
    mLocalizedName = in.readString();
    mRequired = in.readByte() != 0;
    mOptions = in.createTypedArrayList(CatalogCustomPropertyOption.CREATOR);
  }

  @Override
  public void writeToParcel(Parcel dest, int flags)
  {
    dest.writeString(mKey);
    dest.writeString(mLocalizedName);
    dest.writeByte((byte) (mRequired ? 1 : 0));
    dest.writeTypedList(mOptions);
  }

  @Override
  public int describeContents()
  {
    return 0;
  }

  public static final Creator<CatalogCustomProperty> CREATOR = new Creator<CatalogCustomProperty>()
  {
    @Override
    public CatalogCustomProperty createFromParcel(Parcel in)
    {
      return new CatalogCustomProperty(in);
    }

    @Override
    public CatalogCustomProperty[] newArray(int size)
    {
      return new CatalogCustomProperty[size];
    }
  };

  @NonNull
  public String getKey() { return mKey; }

  @NonNull
  public String getLocalizedName() { return mLocalizedName; }

  public boolean isRequired() { return mRequired; }

  @NonNull
  public List<CatalogCustomPropertyOption> getOptions() { return mOptions; }
}
