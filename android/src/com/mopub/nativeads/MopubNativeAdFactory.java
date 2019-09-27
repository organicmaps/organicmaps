package com.mopub.nativeads;

import android.os.SystemClock;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.mapswithme.maps.ads.AdDataAdapter;
import com.mapswithme.maps.ads.CachedMwmNativeAd;
import com.mapswithme.maps.ads.MopubNativeAd;
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

    return null;
  }
}
