package com.mapswithme.maps.ads;

import android.support.annotation.Nullable;

public interface NativeAdError
{
  @Nullable
  String getMessage();

  int getCode();
}
