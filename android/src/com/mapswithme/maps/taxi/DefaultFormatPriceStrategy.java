package com.mapswithme.maps.taxi;

import android.support.annotation.NonNull;

class DefaultFormatPriceStrategy implements FormatPriceStrategy
{
  @NonNull
  @Override
  public String format(@NonNull TaxiInfo.Product product)
  {
    return product.getPrice();
  }
}
