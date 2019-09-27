package com.mapswithme.maps.ads;

import androidx.annotation.NonNull;

/**
 * A common listener to make all interested observers be able to watch for ads cache modifications.
 */
interface OnAdCacheModifiedListener
{
  void onRemoved(@NonNull BannerKey key);
  void onPut(@NonNull BannerKey key);
}
