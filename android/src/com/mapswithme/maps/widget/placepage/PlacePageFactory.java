package com.mapswithme.maps.widget.placepage;

import androidx.annotation.NonNull;
import com.mapswithme.maps.purchase.AdsRemovalPurchaseControllerProvider;

public class PlacePageFactory
{
  @NonNull
  public static PlacePageController createPlacePageController(
      @NonNull AdsRemovalPurchaseControllerProvider provider,
      @NonNull PlacePageController.SlideListener slideListener,
      @NonNull RoutingModeListener routingModeListener)
  {
    return new PlacePageControllerComposite(provider, slideListener, routingModeListener);
  }
}
