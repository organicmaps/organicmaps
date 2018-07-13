package com.mapswithme.maps.ads;

import android.support.annotation.NonNull;
import android.view.View;

import com.mopub.nativeads.BaseNativeAd;

public interface AdRegistrator
{
  void registerView(@NonNull BaseNativeAd ad, @NonNull View view);
  void unregisterView(@NonNull BaseNativeAd ad, @NonNull View view);
}
