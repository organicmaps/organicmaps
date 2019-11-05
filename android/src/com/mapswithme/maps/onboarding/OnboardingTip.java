package com.mapswithme.maps.onboarding;

import androidx.annotation.IntDef;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

public class OnboardingTip
{
  // The order is important, must corresponds to
  // OnboardingTip::Type enum at map/onboarding.hpp.
  public static final int DISCOVER_CATALOG= 0;
  public static final int DOWNLOAD_SAMPLES = 1;
  public static final int BUY_SUBSCRIPTION = 2;

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
}
