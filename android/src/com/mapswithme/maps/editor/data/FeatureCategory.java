package com.mapswithme.maps.editor.data;

import android.os.Parcel;
import android.os.Parcelable;

public class FeatureCategory implements Parcelable
{
  public final int category;
  // Localized category name.
  public final String name;

  public FeatureCategory(int category, String name)
  {
    this.category = category;
    this.name = name;
  }

  public FeatureCategory(Parcel source)
  {
    category = source.readInt();
    name = source.readString();
  }

  @Override
  public int describeContents()
  {
    return 0;
  }

  @Override
  public void writeToParcel(Parcel dest, int flags)
  {
    dest.writeInt(category);
    dest.writeString(name);
  }

  public static final Creator<FeatureCategory> CREATOR = new Creator<FeatureCategory>()
  {
    @Override
    public FeatureCategory createFromParcel(Parcel source)
    {
      return new FeatureCategory(source);
    }

    @Override
    public FeatureCategory[] newArray(int size)
    {
      return new FeatureCategory[size];
    }
  };
}
