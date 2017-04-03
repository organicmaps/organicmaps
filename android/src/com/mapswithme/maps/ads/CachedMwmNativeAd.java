package com.mapswithme.maps.ads;

abstract class CachedMwmNativeAd extends BaseMwmNativeAd
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
}
