package com.mapswithme.maps.widget.placepage;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.mapswithme.maps.R;
import com.mapswithme.maps.guides.GuidesGalleryListener;
import com.mapswithme.maps.purchase.AdsRemovalPurchaseControllerProvider;

public class PlacePageFactory
{
  @NonNull
  public static PlacePageController createCompositePlacePageController(
      @NonNull AdsRemovalPurchaseControllerProvider provider,
      @NonNull PlacePageController.SlideListener slideListener,
      @NonNull RoutingModeListener routingModeListener,
      @Nullable GuidesGalleryListener galleryListener)
  {
    return new PlacePageControllerComposite(provider, slideListener, routingModeListener,
                                            galleryListener);
  }

  @NonNull
  static PlacePageController createRichController(
      @NonNull AdsRemovalPurchaseControllerProvider provider,
      @NonNull PlacePageController.SlideListener listener,
      @Nullable RoutingModeListener routingModeListener)
  {
    return new RichPlacePageController(provider, listener, routingModeListener);
  }

  @NonNull
  static PlacePageController createElevationProfilePlacePageController(
      @NonNull PlacePageController.SlideListener listener)
  {
    ElevationProfileViewRenderer renderer = new ElevationProfileViewRenderer();
    return new SimplePlacePageController(R.id.elevation_profile, renderer, renderer, listener);
  }

  @NonNull
  static PlacePageController createGuidesGalleryController(
      @NonNull PlacePageController.SlideListener listener,
      @Nullable GuidesGalleryListener galleryListener)
  {
    GuidesGalleryViewRenderer renderer = new GuidesGalleryViewRenderer(galleryListener);
    return new SimplePlacePageController(R.id.guides_gallery_bottom_sheet, renderer, renderer,
                                         listener);
  }
}
