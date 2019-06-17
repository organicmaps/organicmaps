package com.mapswithme.maps.widget.placepage;

import android.support.annotation.NonNull;

import com.mapswithme.maps.promo.Promo;
import com.mapswithme.maps.promo.PromoCityGallery;

class CatalogPromoListener implements Promo.Listener
{
  @NonNull
  private final PlacePageView mPlacePage;

  CatalogPromoListener(@NonNull PlacePageView placePage)
  {
    mPlacePage = placePage;
  }

  @Override
  public void onCityGalleryReceived(@NonNull PromoCityGallery gallery)
  {
    mPlacePage.setCatalogPromoGallery(gallery);
  }

  @Override
  public void onErrorReceived()
  {
    mPlacePage.setCatalogPromoGalleryError();
  }
}
