package app.organicmaps.sdk.car.renderer;

import androidx.annotation.NonNull;
import androidx.car.app.CarContext;
import androidx.lifecycle.LifecycleOwner;
import app.organicmaps.sdk.display.DisplayManager;
import app.organicmaps.sdk.location.LocationHelper;

public final class RendererFactory
{
  @NonNull
  public static Renderer create(@NonNull CarContext carContext, @NonNull DisplayManager displayManager,
                                @NonNull LocationHelper locationHelper, @NonNull LifecycleOwner lifecycleOwner)
  {
    return new SurfaceRenderer(carContext, displayManager, locationHelper, lifecycleOwner);
  }
}
