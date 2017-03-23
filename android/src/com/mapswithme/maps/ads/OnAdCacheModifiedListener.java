package com.mapswithme.maps.ads;

import android.support.annotation.NonNull;

interface OnAdCacheModifiedListener
{
  void onRemove(@NonNull String id);
  void onPut(@NonNull String id);
}
