package com.mapswithme.maps.adapter;

import android.support.annotation.NonNull;

public interface AdapterIndexConverter
{
  @NonNull
  AdapterIndexAndPosition getRelativePosition(int absPosition);

  @NonNull
  AdapterIndexAndViewType getRelativeViewType(int absViewType);

  int getAbsoluteViewType(@NonNull AdapterIndexAndViewType relViewType);
}
