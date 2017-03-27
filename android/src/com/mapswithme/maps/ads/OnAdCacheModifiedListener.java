package com.mapswithme.maps.ads;

import android.support.annotation.NonNull;

interface OnAdCacheModifiedListener
{
  void onRemoved(@NonNull String bannerId);
  void onPut(@NonNull String bannerId);
}
