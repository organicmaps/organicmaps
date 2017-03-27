package com.mapswithme.maps.ads;

import android.support.annotation.NonNull;

/**
 * A common listener to make all interested observers be able to watch for ads cache modifications.
 */
interface OnAdCacheModifiedListener
{
  void onRemoved(@NonNull String bannerId);
  void onPut(@NonNull String bannerId);
}
