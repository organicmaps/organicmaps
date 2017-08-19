package com.mapswithme.maps.ads;

import com.mapswithme.maps.Framework;

public class GoogleSearchAd
{
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

  String getAdUnitId() { return mAdUnitId; }
}
