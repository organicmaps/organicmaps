package com.mapswithme.maps.gallery;

import android.os.Parcel;
import android.os.Parcelable;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

public class Image implements Parcelable
{
  @NonNull
  private final String mUrl;
  @NonNull
  private final String mSmallUrl;
  @Nullable
  private String mDescription;
  @Nullable
  private String mUserName;
  @Nullable
  private String mUserAvatar;
  @Nullable
  private String mSource;
  @Nullable
  private Long mDate;

  @SuppressWarnings("unused")
  public Image(@NonNull String url, @NonNull String smallUrl)
  {
    this.mUrl = url;
    this.mSmallUrl = smallUrl;
  }

  protected Image(Parcel in)
  {
    mUrl = in.readString();
    mSmallUrl = in.readString();
    mDescription = in.readString();
    mUserName = in.readString();
    mUserAvatar = in.readString();
    mSource = in.readString();
    mDate = (Long) in.readValue(Long.class.getClassLoader());
  }

  @Override
  public void writeToParcel(Parcel dest, int flags)
  {
    dest.writeString(mUrl);
    dest.writeString(mSmallUrl);
    dest.writeString(mDescription);
    dest.writeString(mUserName);
    dest.writeString(mUserAvatar);
    dest.writeString(mSource);
    dest.writeValue(mDate);
  }

  @Override
  public int describeContents()
  {
    return 0;
  }

  public static final Creator<Image> CREATOR = new Creator<Image>()
  {
    @Override
    public Image createFromParcel(Parcel in)
    {
      return new Image(in);
    }

    @Override
    public Image[] newArray(int size)
    {
      return new Image[size];
    }
  };

  @NonNull
  public String getUrl()
  {
    return mUrl;
  }

  @NonNull
  public String getSmallUrl()
  {
    return mSmallUrl;
  }

  @Nullable
  public String getDescription()
  {
    return mDescription;
  }

  public void setDescription(@Nullable String description)
  {
    this.mDescription = description;
  }

  @Nullable
  String getUserName()
  {
    return mUserName;
  }

  @SuppressWarnings("unused")
  public void setUserName(@Nullable String userName)
  {
    this.mUserName = userName;
  }

  @Nullable
  String getUserAvatar()
  {
    return mUserAvatar;
  }

  @SuppressWarnings("unused")
  public void setUserAvatar(@Nullable String userAvatar)
  {
    this.mUserAvatar = userAvatar;
  }

  @Nullable
  public String getSource()
  {
    return mSource;
  }

  public void setSource(@Nullable String source)
  {
    this.mSource = source;
  }

  @Nullable
  public Long getDate()
  {
    return mDate;
  }

  public void setDate(@Nullable Long date)
  {
    this.mDate = date;
  }
}
