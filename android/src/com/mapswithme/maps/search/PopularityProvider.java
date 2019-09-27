package com.mapswithme.maps.search;

import androidx.annotation.NonNull;

public interface PopularityProvider
{
  @NonNull
  Popularity getPopularity();
}
