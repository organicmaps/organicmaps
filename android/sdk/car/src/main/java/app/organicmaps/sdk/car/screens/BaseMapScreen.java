package app.organicmaps.sdk.car.screens;

import androidx.annotation.NonNull;
import androidx.car.app.CarContext;
import app.organicmaps.sdk.OrganicMaps;
import app.organicmaps.sdk.car.renderer.Renderer;

public abstract class BaseMapScreen extends BaseScreen
{
  @NonNull
  private final Renderer mSurfaceRenderer;

  public BaseMapScreen(@NonNull CarContext carContext, @NonNull OrganicMaps organicMapsContext,
                       @NonNull Renderer surfaceRenderer)
  {
    super(carContext, organicMapsContext);
    mSurfaceRenderer = surfaceRenderer;
  }

  @NonNull
  protected Renderer getSurfaceRenderer()
  {
    return mSurfaceRenderer;
  }
}
