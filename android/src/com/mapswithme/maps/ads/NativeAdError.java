package com.mapswithme.maps.ads;

import androidx.annotation.Nullable;

public interface NativeAdError
{
  @Nullable
  String getMessage();

  int getCode();
}
