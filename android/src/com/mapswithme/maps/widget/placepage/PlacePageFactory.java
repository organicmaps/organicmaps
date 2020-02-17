package com.mapswithme.maps.widget.placepage;

import android.app.Activity;

import androidx.annotation.NonNull;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.purchase.AdsRemovalPurchaseControllerProvider;

public class PlacePageFactory
{
  @NonNull
  public static PlacePageController<MapObject> createPlacePageController(
      @NonNull Activity activity, @NonNull AdsRemovalPurchaseControllerProvider provider,
      @NonNull PlacePageController.SlideListener slideListener,
      @NonNull RoutingModeListener routingModeListener)
  {
    return new PlacePageControllerComposite(activity, provider, slideListener, routingModeListener);
  }
}
