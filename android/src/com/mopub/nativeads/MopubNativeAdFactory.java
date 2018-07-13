package com.mopub.nativeads;

import android.os.SystemClock;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.mapswithme.maps.ads.AdDataAdapter;
import com.mapswithme.maps.ads.CachedMwmNativeAd;
import com.mapswithme.maps.ads.MopubNativeAd;
import com.mopub.nativeads.GooglePlayServicesNative.GooglePlayServicesNativeAd;

public class MopubNativeAdFactory
{
  @Nullable
  public static CachedMwmNativeAd createNativeAd(@NonNull NativeAd ad)
  {
    BaseNativeAd baseAd = ad.getBaseNativeAd();
    if (baseAd instanceof StaticNativeAd)
    {
      return new MopubNativeAd(ad, new AdDataAdapter.StaticAd((StaticNativeAd) baseAd), null,
                               SystemClock.elapsedRealtime());
    }

    if (baseAd instanceof GooglePlayServicesNativeAd
        && ((GooglePlayServicesNativeAd) baseAd).isNativeContentAd())
    {
      return new MopubNativeAd(ad, new GoogleDataAdapter((GooglePlayServicesNativeAd) baseAd),
                               new GoogleAdRegistrator(), SystemClock.elapsedRealtime());
    }

    return null;
  }
}
