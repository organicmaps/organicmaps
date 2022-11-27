package app.organicmaps.car;

import androidx.annotation.NonNull;
import androidx.car.app.model.ActionStrip;
import androidx.car.app.navigation.model.MapController;

public class OMController
{
  @NonNull
  private final SurfaceRenderer mSurfaceRenderer;
  @NonNull
  private final MapController mMapController;
  @NonNull
  private final ActionStrip mActionStrip;

  public OMController(@NonNull SurfaceRenderer surfaceRenderer, @NonNull MapController mapController, @NonNull ActionStrip actionStrip)
  {
    mSurfaceRenderer = surfaceRenderer;
    mMapController = mapController;
    mActionStrip = actionStrip;
  }

  @NonNull
  public SurfaceRenderer getSurfaceRenderer()
  {
    return mSurfaceRenderer;
  }

  @NonNull
  public MapController getMapController()
  {
    return mMapController;
  }

  @NonNull
  public ActionStrip getActionStrip()
  {
    return mActionStrip;
  }
}
