package com.mapswithme.maps.search;

import android.support.annotation.NonNull;

import com.google.android.gms.ads.search.SearchAdView;

class GoogleAdsBanner implements SearchData
{
  @NonNull
  private SearchAdView mAdView;

  GoogleAdsBanner(@NonNull SearchAdView adView)
  {
    this.mAdView = adView;
  }

  @NonNull
  SearchAdView getAdView()
  {
    return mAdView;
  }

  @Override
  public int getItemViewType()
  {
    return SearchResultTypes.TYPE_GOOGLE_ADS;
  }
}
