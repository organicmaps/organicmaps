package com.mapswithme.maps.ads;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

public class Factory
{
  @NonNull
  public static NativeAdLoader createFacebookAdLoader(@Nullable OnAdCacheModifiedListener cacheListener,
                                        @Nullable AdTracker tracker)
  {
    return new FacebookAdsLoader(cacheListener, tracker);
  }
}
