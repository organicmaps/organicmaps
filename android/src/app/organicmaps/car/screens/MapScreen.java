package app.organicmaps.car.screens;

import androidx.annotation.NonNull;
import androidx.car.app.CarContext;
import androidx.car.app.Screen;
import androidx.car.app.model.ActionStrip;
import androidx.car.app.navigation.model.MapController;

import app.organicmaps.car.OMController;
import app.organicmaps.car.SurfaceRenderer;

public abstract class MapScreen extends Screen
{
  @NonNull
  private final OMController mMapController;

  public MapScreen(@NonNull CarContext carContext, @NonNull OMController mapController)
  {
    super(carContext);
    mMapController = mapController;
  }

  @NonNull
  public OMController getOMController()
  {
    return mMapController;
  }

  @NonNull
  public SurfaceRenderer getSurfaceRenderer()
  {
    return mMapController.getSurfaceRenderer();
  }

  @NonNull
  public MapController getMapController()
  {
    return mMapController.getMapController();
  }

  @NonNull
  public ActionStrip getActionStrip()
  {
    return mMapController.getActionStrip();
  }
}
