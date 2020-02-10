package com.mapswithme.maps.widget.placepage;

import android.app.Activity;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.purchase.AdsRemovalPurchaseControllerProvider;

public class PlacePageControllerFactory
{
  @NonNull
  public static PlacePageController<MapObject> createBottomSheetPlacePageController(
      @NonNull Activity activity, @NonNull AdsRemovalPurchaseControllerProvider provider,
      @NonNull PlacePageController.SlideListener listener,
      @Nullable RoutingModeListener routingModeListener)
  {
    return new BottomSheetPlacePageController(activity, provider, listener, routingModeListener);
  }

  @NonNull
  public static PlacePageController<MapObject> createElevationProfileBottomSheetController(
      @NonNull Activity activity, @NonNull PlacePageController.SlideListener listener)
  {
    return new ElevationProfileBottomSheetController(activity, listener);
  }
}
