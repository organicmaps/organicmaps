package com.mapswithme.maps.ads;

import androidx.annotation.Nullable;

class MopubAdError implements NativeAdError
{
  @Nullable
  private final String mMessage;

  MopubAdError(@Nullable String message)
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
