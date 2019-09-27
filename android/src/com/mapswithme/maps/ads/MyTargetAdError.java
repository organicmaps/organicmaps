package com.mapswithme.maps.ads;

import androidx.annotation.Nullable;


class MyTargetAdError implements NativeAdError
{
  @Nullable
  private final String mMessage;

  MyTargetAdError(@Nullable String message)
  {
    mMessage = message;
  }

  @Nullable
  @Override
  public String getMessage()
  {
    return mMessage;
  }

  @Override
  public int getCode()
  {
    return 0;
  }
}
