package com.mapswithme.maps.ads;

import android.support.annotation.NonNull;

interface OnAdCacheModifiedListener
{
  void onRemoved(@NonNull String id);
  void onPut(@NonNull String id);
}
