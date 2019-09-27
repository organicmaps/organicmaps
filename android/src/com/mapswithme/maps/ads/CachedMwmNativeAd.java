package com.mapswithme.maps.ads;

import androidx.annotation.NonNull;

public abstract class CachedMwmNativeAd extends BaseMwmNativeAd
{
  private final long mLoadedTime;

  CachedMwmNativeAd(long loadedTime)
  {
    mLoadedTime = loadedTime;
  }

  long getLoadedTime()
  {
    return mLoadedTime;
  }

  abstract void detachAdListener();

  abstract void attachAdListener(@NonNull Object listener);
}
