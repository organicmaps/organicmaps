package app.organicmaps.car.renderer;

import androidx.annotation.NonNull;
import androidx.annotation.RequiresApi;
import androidx.car.app.CarContext;
import androidx.lifecycle.LifecycleOwner;
import app.organicmaps.sdk.MapController;
import app.organicmaps.sdk.MapView;
import app.organicmaps.sdk.display.DisplayManager;
import app.organicmaps.sdk.display.DisplayType;
import app.organicmaps.sdk.location.LocationHelper;
import app.organicmaps.sdk.util.log.Logger;

@RequiresApi(23)
class SurfaceRenderer extends Renderer
{
  private static final String TAG = SurfaceRenderer.class.getSimpleName();

  @NonNull
  private final MapController mMapController;

  @NonNull
  private final SurfaceCallback mSurfaceCallback;

  public SurfaceRenderer(@NonNull CarContext carContext, @NonNull DisplayManager displayManager,
                         @NonNull LocationHelper locationHelper, @NonNull LifecycleOwner lifecycleOwner)
  {
    super(carContext, displayManager, locationHelper, lifecycleOwner);

    mMapController = new MapController(new MapView(carContext, DisplayType.Car), locationHelper,
                                       getMapRenderingListener(), null, false);
    mLifecycleOwner.getLifecycle().addObserver(mMapController);
    mSurfaceCallback = new SurfaceCallback(mCarContext, mMapController);
    setSurfaceCallback(mSurfaceCallback);
  }

  @Override
  public void onResume(@NonNull LifecycleOwner owner)
  {
    Logger.d(TAG);
    if (mDisplayManager.isCarDisplayUsed())
      mMapController.updateMyPositionRoutingOffset(0);
  }

  @Override
  public void enable()
  {
    super.enable();

    mMapController.onStart(mLifecycleOwner);
    mMapController.updateMyPositionRoutingOffset(0);
    mSurfaceCallback.startPresenting();
  }

  @Override
  public void disable()
  {
    super.disable();

    mMapController.onPause(mLifecycleOwner);
    mSurfaceCallback.stopPresenting();
    mMapController.onStop(mLifecycleOwner);
  }
}
