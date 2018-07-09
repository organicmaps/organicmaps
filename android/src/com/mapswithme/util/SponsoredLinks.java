package com.mapswithme.util;

import android.support.annotation.NonNull;

public class SponsoredLinks
{
  @NonNull
  private final String mDeepLink;
  @NonNull
  private final String mUniversalLink;

  public SponsoredLinks(@NonNull String deepLink, @NonNull String universalLink)
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
