package app.organicmaps.sdk.car.renderer;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.car.app.CarContext;
import androidx.core.util.Consumer;
import androidx.lifecycle.LifecycleOwner;
import app.organicmaps.sdk.display.DisplayManager;
import app.organicmaps.sdk.location.LocationHelper;
import app.organicmaps.sdk.widgets.speedlimit.CombinedSpeedViewController;

final class SurfaceRenderer extends RendererImpl
{
  @NonNull
  private final SurfaceCallback mSurfaceCallback;

  public SurfaceRenderer(@NonNull CarContext carContext, @NonNull DisplayManager displayManager,
                         @NonNull LocationHelper locationHelper, @NonNull LifecycleOwner lifecycleOwner)
  {
    super(carContext, displayManager, locationHelper, lifecycleOwner);

    mSurfaceCallback =
        new SurfaceCallback(mCarContext, mLifecycleOwner, locationHelper, getMapRenderingListener(), displayManager);
    setSurfaceCallback(mSurfaceCallback);
  }

  @Override
  public void enable()
  {
    super.enable();
    mSurfaceCallback.startPresenting();
  }

  @Override
  public void disable()
  {
    super.disable();
    mSurfaceCallback.stopPresenting();
  }

  @Override
  public void onDestroy(@NonNull LifecycleOwner owner)
  {
    mSurfaceCallback.destroy();
    super.onDestroy(owner);
  }

  @Override
  @Nullable
  public CombinedSpeedViewController getSpeedLimitController()
  {
    return mSurfaceCallback.getSpeedLimitController();
  }

  @Override
  public void setSpeedLimitControllerListener(@Nullable Consumer<CombinedSpeedViewController> listener)
  {
    mSurfaceCallback.setSpeedLimitControllerListener(listener);
  }
}
