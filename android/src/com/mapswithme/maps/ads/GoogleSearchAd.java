package com.mapswithme.maps.ads;

import android.support.annotation.NonNull;

import com.mapswithme.maps.Framework;

public class GoogleSearchAd
{
  @NonNull
  private String mAdUnitId = "";

  public GoogleSearchAd()
  {
    Banner[] banners = Framework.nativeGetSearchBanners();
    if (banners == null)
      return;

    for (Banner b : banners)
    {
      if (b.getProvider().equals(Providers.GOOGLE))
      {
        mAdUnitId = b.getId();
        break;
      }
    }
  }

  @NonNull
  public String getAdUnitId() { return mAdUnitId; }
}
