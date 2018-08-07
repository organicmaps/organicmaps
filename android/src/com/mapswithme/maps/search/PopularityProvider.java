package com.mapswithme.maps.search;

import android.support.annotation.NonNull;

public interface PopularityProvider
{
  @NonNull
  Popularity getPopularity();
}
