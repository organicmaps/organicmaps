package com.mapswithme.maps.bookmarks.data;

import android.os.Parcel;
import android.os.Parcelable;
import android.support.annotation.Nullable;

public final class Banner implements Parcelable
{
  public static final Banner EMPTY = new Banner(null, null, null, null, null, null);

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
  @Nullable
  private final String mTitle;
  @Nullable
  private final String mMessage;
  @Nullable
  private final String mIconUrl;
  @Nullable
  private final String mUrl;
  @Nullable
  private final String mTypes;

  public Banner(@Nullable String id, @Nullable String title, @Nullable String message,
                @Nullable String iconUrl, @Nullable String url, @Nullable String types)
  {
    mId = id;
    mTitle = title;
    mMessage = message;
    mIconUrl = iconUrl;
    mUrl = url;
    mTypes = types;
  }

  protected Banner(Parcel in)
  {
    mId = in.readString();
    mTitle = in.readString();
    mMessage = in.readString();
    mIconUrl = in.readString();
    mUrl = in.readString();
    mTypes = in.readString();
  }

  @Nullable
  public String getId()
  {
    return mId;
  }

  @Nullable
  public String getTitle()
  {
    return mTitle;
  }

  @Nullable
  public String getMessage()
  {
    return mMessage;
  }

  @Nullable
  public String getIconUrl()
  {
    return mIconUrl;
  }

  @Nullable
  public String getUrl()
  {
    return mUrl;
  }

  @Nullable
  public String getTypes()
  {
    return mTypes;
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
    dest.writeString(mTitle);
    dest.writeString(mMessage);
    dest.writeString(mIconUrl);
    dest.writeString(mUrl);
    dest.writeString(mTypes);
  }

  @Override
  public String toString()
  {
    return "Banner{" +
           "mId='" + mId + '\'' +
           ", mTitle='" + mTitle + '\'' +
           ", mMessage='" + mMessage + '\'' +
           ", mIconUrl='" + mIconUrl + '\'' +
           ", mUrl='" + mUrl + '\'' +
           ", mTypes='" + mTypes + '\'' +
           '}';
  }
}
