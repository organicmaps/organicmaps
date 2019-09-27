package com.mapswithme.maps.gallery;

import androidx.annotation.NonNull;

public interface ItemSelectedListener<I>
{
  void onItemSelected(@NonNull I item, int position);

  void onMoreItemSelected(@NonNull I item);

  void onActionButtonSelected(@NonNull I item, int position);
}
