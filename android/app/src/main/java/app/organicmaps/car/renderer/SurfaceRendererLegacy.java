package app.organicmaps.car.renderer;

import static app.organicmaps.sdk.display.DisplayType.Car;

import androidx.annotation.NonNull;
import androidx.car.app.CarContext;
import androidx.lifecycle.LifecycleOwner;
import app.organicmaps.sdk.Map;
import app.organicmaps.sdk.display.DisplayManager;
import app.organicmaps.sdk.location.LocationHelper;
import app.organicmaps.sdk.util.log.Logger;

class SurfaceRendererLegacy extends Renderer
{
  private static final String TAG = SurfaceRendererLegacy.class.getSimpleName();

  @NonNull
  private final Map mMap = new Map(Car);

  public SurfaceRendererLegacy(@NonNull CarContext carContext, @NonNull DisplayManager displayManager,
                               @NonNull LocationHelper locationHelper, @NonNull LifecycleOwner lifecycleOwner)
  {
    super(carContext, displayManager, locationHelper, lifecycleOwner);
    setSurfaceCallback(new SurfaceCallbackLegacy(mCarContext, mMap, mLocationHelper));
    mMap.setMapRenderingListener(getMapRenderingListener());
  }

  @Override
  public void onCreate(@NonNull LifecycleOwner owner)
  {
    super.onCreate(owner);
    mMap.onCreate(false);
  }

  @Override
  public void onStart(@NonNull LifecycleOwner owner)
  {
    Logger.d(TAG);
    if (mDisplayManager.isCarDisplayUsed())
      mMap.onStart();
  }

  @Override
  public void onResume(@NonNull LifecycleOwner owner)
  {
    Logger.d(TAG);
    if (mDisplayManager.isCarDisplayUsed())
    {
      mMap.onResume();
      mMap.updateMyPositionRoutingOffset(0);
    }
  }

  @Override
  public void onPause(@NonNull LifecycleOwner owner)
  {
    Logger.d(TAG);
    if (mDisplayManager.isCarDisplayUsed())
      mMap.onPause();
  }

  @Override
  public void onStop(@NonNull LifecycleOwner owner)
  {
    Logger.d(TAG);
    if (mDisplayManager.isCarDisplayUsed())
      mMap.onStop();
  }

  @Override
  public void enable()
  {
    super.enable();

    mMap.onStart();
    mMap.setMapRenderingListener(getMapRenderingListener());
    mMap.updateMyPositionRoutingOffset(0);
  }

  @Override
  public void disable()
  {
    super.disable();

    mMap.onPause();
    mMap.onSurfaceDestroyed(false);
    mMap.onStop();
    mMap.setMapRenderingListener(null);
  }
}
