package com.mapswithme.maps.uber;

import android.support.annotation.NonNull;

public class UberLinks
{
  @NonNull
  private final String mDeepLink;
  @NonNull
  private final String mUniversalLink;

  public UberLinks(@NonNull String deepLink, @NonNull String universalLink)
  {
    mDeepLink = deepLink;
    mUniversalLink = universalLink;
  }

  @NonNull
  public String getDeepLink()
  {
    return mDeepLink;
  }

  @NonNull
  public String getUniversalLink()
  {
    return mUniversalLink;
  }
}
