package com.mapswithme.maps.onboarding;

import android.os.Parcel;
import android.os.Parcelable;

import androidx.annotation.IntDef;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

public class OnboardingTip implements Parcelable
{
  // The order is important, must corresponds to
  // OnboardingTip::Type enum at map/onboarding.hpp.
  public static final int DISCOVER_CATALOG= 0;
  public static final int DOWNLOAD_SAMPLES = 1;
  public static final int BUY_SUBSCRIPTION = 2;

  protected OnboardingTip(Parcel in)
  {
    mType = in.readInt();
    mUrl = in.readString();
  }

  @Override
  public void writeToParcel(Parcel dest, int flags)
  {
    dest.writeInt(mType);
    dest.writeString(mUrl);
  }

  @Retention(RetentionPolicy.SOURCE)
  @IntDef({ DISCOVER_CATALOG, DOWNLOAD_SAMPLES, BUY_SUBSCRIPTION})
  @interface ScreenType {}

  @ScreenType
  private int mType;
  @NonNull
  private String mUrl;

  @SuppressWarnings("unused")
  OnboardingTip(@ScreenType int type, @NonNull String url)
  {
    mType = type;
    mUrl = url;
  }

  @ScreenType
  public int getType()
  {
    return mType;
  }

  @NonNull
  public String getUrl()
  {
    return mUrl;
  }

  @Nullable
  public static OnboardingTip get()
  {
    return nativeGetTip();
  }

  @Nullable
  private static native OnboardingTip nativeGetTip();

  public static final Creator<OnboardingTip> CREATOR = new Creator<OnboardingTip>()
  {
    @Override
    public OnboardingTip createFromParcel(Parcel in)
    {
      return new OnboardingTip(in);
    }

    @Override
    public OnboardingTip[] newArray(int size)
    {
      return new OnboardingTip[size];
    }
  };

  @Override
  public int describeContents()
  {
    return 0;
  }
}
