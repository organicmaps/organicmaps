package com.mapswithme.maps.adapter;

import androidx.annotation.NonNull;

public interface AdapterPositionConverter
{
  @NonNull
  AdapterIndexAndPosition toRelativePositionAndAdapterIndex(int absPosition);

  @NonNull
  AdapterIndexAndViewType toRelativeViewTypeAndAdapterIndex(int absViewType);

  int toAbsoluteViewType(int relViewType, int adapterIndex);
}
