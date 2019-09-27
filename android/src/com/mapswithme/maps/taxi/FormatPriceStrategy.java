package com.mapswithme.maps.taxi;

import androidx.annotation.NonNull;

interface FormatPriceStrategy
{
  @NonNull
  String format(@NonNull TaxiInfo.Product product);
}
