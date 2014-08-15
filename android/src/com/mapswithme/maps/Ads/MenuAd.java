package com.mapswithme.maps.Ads;

import android.graphics.Bitmap;
import android.os.Parcel;
import android.os.Parcelable;

public class MenuAd implements Parcelable
{
  private String mIconUrl;
  private String mTitle;
  private String mHexColor;
  private String mId;
  private String mAppUrl;
  private String mWebUrl;
  private Bitmap mIcon;

  public MenuAd(String iconUrl, String title, String hexColor, String id, String appUrl, String webUrl, Bitmap icon)
  {
    mIconUrl = iconUrl;
    mTitle = title;
    mHexColor = hexColor;
    mId = id;
    mAppUrl = appUrl;
    mWebUrl = webUrl;
    mIcon = icon;
  }

  public MenuAd(Parcel source)
  {
    mIconUrl = source.readString();
    mTitle = source.readString();
    mHexColor = source.readString();
    mId = source.readString();
    mAppUrl = source.readString();
    mWebUrl = source.readString();
    mIcon = source.readParcelable(Bitmap.class.getClassLoader());
  }

  @Override
  public String toString()
  {
    return mIconUrl + " " + mTitle + " " + mHexColor + " " + mId + " " + mAppUrl + " " + mWebUrl;
  }

  public String getAppUrl()
  {
    return mAppUrl;
  }

  public String getHexColor()
  {
    return mHexColor;
  }

  public String getIconUrl()
  {
    return mIconUrl;
  }

  public String getId()
  {
    return mId;
  }

  public String getTitle()
  {
    return mTitle;
  }

  public String getWebUrl()
  {
    return mWebUrl;
  }

  public Bitmap getIcon()
  {
    return mIcon;
  }

  @Override
  public int describeContents()
  {
    return 0;
  }

  @Override
  public void writeToParcel(Parcel dest, int flags)
  {
    dest.writeString(mIconUrl);
    dest.writeString(mTitle);
    dest.writeString(mHexColor);
    dest.writeString(mId);
    dest.writeString(mAppUrl);
    dest.writeString(mWebUrl);
    dest.writeParcelable(mIcon, 0);
  }

  public static final Creator<MenuAd> CREATOR = new Creator<MenuAd>()
  {
    @Override
    public MenuAd createFromParcel(Parcel source)
    {
      return new MenuAd(source);
    }

    @Override
    public MenuAd[] newArray(int size)
    {
      return new MenuAd[size];
    }
  };
}