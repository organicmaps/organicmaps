package com.mapswithme.maps.bookmarks.data;

import android.os.Parcel;
import android.os.Parcelable;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

public final class Banner implements Parcelable
{
  private final boolean mIsActive;
  @Nullable
  private final String mTitle;
  @Nullable
  private final String mMessage;
  @Nullable
  private final String mIconUrl;
  @Nullable
  private final String mUrl;

  @NonNull
  public static Banner empty()
  {
    return new Banner(false, "", "", "", "");
  }

  public Banner(boolean isActive, @Nullable String title, @Nullable String message,
                @Nullable String iconUrl, @Nullable String url)
  {
    mIsActive = isActive;
    mTitle = title;
    mMessage = message;
//    mIconUrl = iconUrl;
    mIconUrl = "https://lh6.ggpht.com/bVwOOcO1jm_bfvqtkUDEyyOl2PZ-ZLaxqzylW5NtM2NHSlLQAnC1t45gf6d6JX07XQ=w300";
    mUrl = url;
  }

  protected Banner(Parcel in)
  {
    mIsActive = in.readByte() != 0;
    mTitle = in.readString();
    mMessage = in.readString();
    mIconUrl = in.readString();
    mUrl = in.readString();
  }

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

  public boolean isActive()
  {
    return mIsActive;
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

  @Override
  public int describeContents()
  {
    return 0;
  }

  @Override
  public void writeToParcel(Parcel dest, int flags)
  {
    dest.writeByte((byte) (mIsActive ? 1 : 0));
    dest.writeString(mTitle);
    dest.writeString(mMessage);
    dest.writeString(mIconUrl);
    dest.writeString(mUrl);
  }
}
