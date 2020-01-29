package com.mapswithme.maps.ads;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

public class Factory
{
  @NonNull
  static NativeAdLoader createLoaderForBanner(@NonNull Banner banner,
                                              @Nullable OnAdCacheModifiedListener cacheListener,
                                              @Nullable AdTracker tracker)
  {
    String provider = banner.getProvider();
    switch (provider)
    {
      case Providers.MY_TARGET:
        return new MyTargetAdsLoader(cacheListener, tracker);
      case Providers.MOPUB:
        return new MopubNativeDownloader(cacheListener, tracker);
      case Providers.GOOGLE:
        throw new AssertionError("Not implemented yet");
      default:
        throw new AssertionError("Unknown ads provider: " + provider);
    }
  }

  @NonNull
  public static CompoundNativeAdLoader createCompoundLoader(
      @Nullable OnAdCacheModifiedListener cacheListener, @Nullable AdTracker tracker)
  {
    return new CompoundNativeAdLoader(cacheListener, tracker);
  }
}
