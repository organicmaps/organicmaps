package com.mapswithme.maps.ads;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

public class Factory
{
  @NonNull
  private static NativeAdLoader createFacebookAdLoader(@Nullable OnAdCacheModifiedListener cacheListener,
                                        @Nullable AdTracker tracker)
  {
    return new FacebookAdsLoader(cacheListener, tracker);
  }

  @NonNull
  private static NativeAdLoader createMyTargetAdLoader(@Nullable OnAdCacheModifiedListener cacheListener,
                                                      @Nullable AdTracker tracker)
  {
    return new MyTargetAdsLoader(cacheListener, tracker);
  }

  @NonNull
  static NativeAdLoader createLoaderForBanner(@NonNull Banner banner,
                                                     @Nullable OnAdCacheModifiedListener cacheListener,
                                                     @Nullable AdTracker tracker)
  {
    String provider = banner.getProvider();
    if (provider.equals(Providers.FACEBOOK))
      return createFacebookAdLoader(cacheListener, tracker);
    else
      return createMyTargetAdLoader(cacheListener, tracker);
  }

  @NonNull
  public static CompoundNativeAdLoader createCompoundLoader(
      @Nullable OnAdCacheModifiedListener cacheListener, @Nullable AdTracker tracker)
  {
    return new CompoundNativeAdLoader(cacheListener, tracker);
  }
}
