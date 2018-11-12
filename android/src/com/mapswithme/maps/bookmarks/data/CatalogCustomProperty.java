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

  @NonNull
  public String getKey() { return mKey; }

  @NonNull
  public String getLocalizedName() { return mLocalizedName; }

  public boolean isRequired() { return mRequired; }

  @NonNull
  public List<CatalogCustomPropertyOption> getOptions() { return mOptions; }

  @Override
  public int describeContents()
  {
    return 0;
  }

  @Override
  public void writeToParcel(Parcel dest, int flags)
  {
    dest.writeString(this.mKey);
    dest.writeString(this.mLocalizedName);
    dest.writeByte(this.mRequired ? (byte) 1 : (byte) 0);
    dest.writeList(this.mOptions);
  }

  protected CatalogCustomProperty(Parcel in)
  {
    this.mKey = in.readString();
    this.mLocalizedName = in.readString();
    this.mRequired = in.readByte() != 0;
    this.mOptions = new ArrayList<>();
    in.readList(this.mOptions, CatalogCustomPropertyOption.class.getClassLoader());
  }

  public static final Creator<CatalogCustomProperty> CREATOR = new Creator<CatalogCustomProperty>()
  {
    @Override
    public CatalogCustomProperty createFromParcel(Parcel source)
    {
      return new CatalogCustomProperty(source);
    }

    @Override
    public CatalogCustomProperty[] newArray(int size)
    {
      return new CatalogCustomProperty[size];
    }
  };
}
