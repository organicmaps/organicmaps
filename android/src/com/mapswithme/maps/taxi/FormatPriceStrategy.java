package com.mapswithme.maps.taxi;

import android.support.annotation.NonNull;

interface FormatPriceStrategy
{
  @NonNull
  String format(@NonNull TaxiInfo.Product product);
}
