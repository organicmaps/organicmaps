package com.mapswithme.maps.taxi;

import android.support.annotation.NonNull;

public class TaxiLinks
{
  @NonNull
  private final String mDeepLink;
  @NonNull
  private final String mUniversalLink;

  public TaxiLinks(@NonNull String deepLink, @NonNull String universalLink)
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
