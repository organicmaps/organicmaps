package com.mapswithme.maps.discovery;

import android.support.annotation.NonNull;

import com.mapswithme.maps.base.BaseSponsoredAdapter;
import com.mapswithme.maps.viator.OfflineViatorAdapter;
import com.mapswithme.maps.viator.ViatorAdapter;
import com.mapswithme.maps.viator.ViatorProduct;

class DiscoveryAdapterFactory
{
  @NonNull
  static BaseSponsoredAdapter createViatorLoadingAdapter()
  {
    return new ViatorAdapter(null, false, null);
  }

  @NonNull
  static BaseSponsoredAdapter createViatorOfflineAdapter()
  {
    return new OfflineViatorAdapter(null);
  }

  @NonNull
  static BaseSponsoredAdapter createViatorAdapter(@NonNull ViatorProduct[] products)
  {
    return new ViatorAdapter(products, null, null, false);
  }
}
