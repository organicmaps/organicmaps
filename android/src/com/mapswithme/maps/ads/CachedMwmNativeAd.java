package com.mapswithme.maps.ads;

import android.support.annotation.NonNull;

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

  abstract void detachAdListener();

  abstract void attachAdListener(@NonNull Object listener);
}
