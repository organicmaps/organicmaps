package com.mapswithme.maps.bookmarks.data;

import android.graphics.Color;
import android.os.Parcel;
import android.os.Parcelable;
import android.support.annotation.NonNull;

public class CatalogTag implements Parcelable
{
  @NonNull
  private final String mId;

  @NonNull
  private final String mLocalizedName;

  private final int mColor;

  public CatalogTag(@NonNull String id, @NonNull String localizedName, float r, float g, float b)
  {
    mId = id;
    mLocalizedName = localizedName;
    mColor = Color.rgb((int)(r * 255), (int)(g * 255), (int)(b * 255));
  }

  @NonNull
  public String getId() { return mId; }

  @NonNull
  public String getLocalizedName() { return mLocalizedName; }

  public int getColor() { return mColor; }

  @Override
  public int describeContents()
  {
    return 0;
  }

  @Override
  public void writeToParcel(Parcel dest, int flags)
  {
    dest.writeString(this.mId);
    dest.writeString(this.mLocalizedName);
    dest.writeInt(this.mColor);
  }

  protected CatalogTag(Parcel in)
  {
    this.mId = in.readString();
    this.mLocalizedName = in.readString();
    this.mColor = in.readInt();
  }

  public static final Creator<CatalogTag> CREATOR = new Creator<CatalogTag>()
  {
    @Override
    public CatalogTag createFromParcel(Parcel source)
    {
      return new CatalogTag(source);
    }

    @Override
    public CatalogTag[] newArray(int size)
    {
      return new CatalogTag[size];
    }
  };

  @Override
  public boolean equals(Object o)
  {
    if (this == o) return true;
    if (o == null || getClass() != o.getClass()) return false;
    CatalogTag that = (CatalogTag) o;
    return mId.equals(that.mId) || mId.equals(that.mId);
  }

  @Override
  public int hashCode()
  {
    return mId.hashCode();
  }

  @Override
  public String toString()
  {
    final StringBuilder sb = new StringBuilder("CatalogTag{");
    sb.append("mId='").append(mId).append('\'');
    sb.append(", mLocalizedName='").append(mLocalizedName).append('\'');
    sb.append('}');
    return sb.toString();
  }
}
