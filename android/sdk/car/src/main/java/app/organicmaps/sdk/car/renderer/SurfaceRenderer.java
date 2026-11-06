package app.organicmaps.sdk.car.renderer;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.car.app.CarContext;
import androidx.lifecycle.LifecycleOwner;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.display.DisplayManager;
import app.organicmaps.sdk.location.LocationHelper;

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
  public void setCurrentSpeed(int speed, @NonNull String units)
  {
    mSurfaceCallback.setCurrentSpeed(speed, units);
  }

  @Override
  public void setSpeedLimit(@Nullable Integer speedLimit)
  {
    mSurfaceCallback.setSpeedLimit(speedLimit);
  }

  @Override
  public void setSpeedLimitSignType(@Framework.SpeedLimitSignType int type)
  {
    mSurfaceCallback.setSpeedLimitSignType(type);
  }

  @Override
  public void setAlertForced(boolean alertForced)
  {
    mSurfaceCallback.setAlertForced(alertForced);
  }
}
