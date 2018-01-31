package com.mapswithme.maps.search;

import android.support.annotation.Nullable;

interface HotelsFilterHolder
{
  @Nullable
  HotelsFilter getHotelsFilter();
  @Nullable
  BookingFilterParams getFilterParams();
}
