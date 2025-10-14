package app.organicmaps.car.screens.base;

import androidx.annotation.NonNull;
import androidx.car.app.CarContext;
import app.organicmaps.car.renderer.Renderer;

public abstract class BaseMapScreen extends BaseScreen
{
  @NonNull
  private final Renderer mSurfaceRenderer;

  public BaseMapScreen(@NonNull CarContext carContext, @NonNull Renderer surfaceRenderer)
  {
    super(carContext);
    mSurfaceRenderer = surfaceRenderer;
  }

  @NonNull
  protected Renderer getSurfaceRenderer()
  {
    return mSurfaceRenderer;
  }
}
