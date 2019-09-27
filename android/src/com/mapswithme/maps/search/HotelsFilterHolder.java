package com.mapswithme.maps.search;

import androidx.annotation.Nullable;

interface HotelsFilterHolder
{
  @Nullable
  HotelsFilter getHotelsFilter();
  @Nullable
  BookingFilterParams getFilterParams();
}
