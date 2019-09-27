package com.mapswithme.maps.ads;

import androidx.annotation.CallSuper;
import androidx.annotation.Nullable;

abstract class BaseNativeAdLoader implements NativeAdLoader
{
  @Nullable
  private NativeAdListener mAdListener;

  @Override
  public void setAdListener(@Nullable NativeAdListener adListener)
  {
    mAdListener = adListener;
  }

  @Nullable
  NativeAdListener getAdListener()
  {
    return mAdListener;
  }

  @CallSuper
  @Override
  public void cancel()
  {
    setAdListener(null);
  }
}
