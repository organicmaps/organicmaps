package app.organicmaps.widget.placepage;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import app.organicmaps.R;

public class PlacePageFactory
{
  @NonNull
  public static PlacePageController createCompositePlacePageController(
      @NonNull PlacePageController.SlideListener slideListener,
      @NonNull RoutingModeListener routingModeListener)
  {
    return new PlacePageControllerComposite(slideListener, routingModeListener);
  }

  @NonNull
  static PlacePageController createRichController(
      @NonNull PlacePageController.SlideListener listener,
      @Nullable RoutingModeListener routingModeListener)
  {
    return new RichPlacePageController(listener, routingModeListener);
  }

  @NonNull
  static PlacePageController createElevationProfilePlacePageController(
      @NonNull PlacePageController.SlideListener listener)
  {
    ElevationProfileViewRenderer renderer = new ElevationProfileViewRenderer();
    return new SimplePlacePageController(R.id.elevation_profile, renderer, renderer, listener);
  }
}
