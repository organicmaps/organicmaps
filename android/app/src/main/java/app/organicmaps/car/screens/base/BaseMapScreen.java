package app.organicmaps.car.screens.base;

import androidx.annotation.NonNull;
import androidx.car.app.CarContext;
import app.organicmaps.car.SurfaceRenderer;

public abstract class BaseMapScreen extends BaseScreen
{
  @NonNull
  private final SurfaceRenderer mSurfaceRenderer;

  public BaseMapScreen(@NonNull CarContext carContext, @NonNull SurfaceRenderer surfaceRenderer)
  {
    super(carContext);
    mSurfaceRenderer = surfaceRenderer;
  }

  @NonNull
  protected SurfaceRenderer getSurfaceRenderer()
  {
    return mSurfaceRenderer;
  }
}
