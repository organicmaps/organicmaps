package com.mapswithme.maps.Ads;

import android.os.Parcel;
import android.os.Parcelable;

public class Banner implements Parcelable
{
  private String mUrl;
  private boolean mShowInLite;
  private boolean mShowInPro;
  private int mLaunchNumber;
  private int mForegroundTime;
  private int mAppVersion;
  private String mId;

  public Banner(String url, boolean showInLite, boolean showInPro, int launchNum, int fgTime, int appVersion, String id)
  {
    mUrl = url;
    mShowInLite = showInLite;
    mShowInPro = showInPro;
    mLaunchNumber = launchNum;
    mForegroundTime = fgTime;
    mAppVersion = appVersion;
    mId = id;
  }

  public Banner(Parcel source)
  {
    mUrl = source.readString();
    mShowInLite = source.readByte() == 1;
    mShowInPro = source.readByte() == 1;
    mLaunchNumber = source.readInt();
    mForegroundTime = source.readInt();
    mAppVersion = source.readInt();
    mId = source.readString();
  }

  public int getFgTime()
  {
    return mForegroundTime;
  }

  public boolean getShowInPro()
  {
    return mShowInPro;
  }

  public String getUrl()
  {
    return mUrl;
  }

  public int getLaunchNumber()
  {
    return mLaunchNumber;
  }

  public boolean getShowInLite()
  {
    return mShowInLite;
  }

  public int getAppVersion()
  {
    return mAppVersion;
  }

  public String getId()
  {
    return mId;
  }

  @Override
  public String toString()
  {
    return "Url " + mUrl + ", showLite " + mShowInLite + ", showPro " + mShowInPro + ", lanuchNum " +
        mLaunchNumber + ", fgTime " + mForegroundTime + ", appVersion " + mAppVersion + ", id " + mId;
  }

  @Override
  public int describeContents()
  {
    return 0;
  }

  @Override
  public void writeToParcel(Parcel dest, int flags)
  {
    dest.writeString(mUrl);
    dest.writeByte((byte) (mShowInLite ? 1 : 0));
    dest.writeByte((byte) (mShowInPro ? 1 : 0));
    dest.writeInt(mLaunchNumber);
    dest.writeInt(mForegroundTime);
    dest.writeInt(mAppVersion);
    dest.writeString(mId);
  }

  public static final Creator<Banner> CREATOR = new Creator<Banner>()
  {
    @Override
    public Banner createFromParcel(Parcel source)
    {
      return new Banner(source);
    }

    @Override
    public Banner[] newArray(int size)
    {
      return new Banner[size];
    }
  };
}