package com.mapswithme.maps.gallery;

import android.support.annotation.NonNull;

public interface ItemSelectedListener<I>
{
  void onItemSelected(@NonNull I item);

  void onMoreItemSelected(@NonNull I item);

  void onActionButtonSelected(@NonNull I item);
}
