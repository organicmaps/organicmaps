package app.organicmaps.sdk.car.renderer;

import android.graphics.Rect;
import android.hardware.display.VirtualDisplay;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.car.app.CarContext;
import androidx.car.app.SurfaceContainer;
import androidx.core.content.ContextCompat;
import androidx.lifecycle.LifecycleOwner;
import app.organicmaps.sdk.MapRenderingListener;
import app.organicmaps.sdk.display.DisplayManager;
import app.organicmaps.sdk.location.LocationHelper;
import app.organicmaps.sdk.util.log.Logger;
import java.util.Objects;

final class SurfaceCallback extends SurfaceCallbackBase
{
  private static final String TAG = SurfaceCallback.class.getSimpleName();

  private static final String VIRTUAL_DISPLAY_NAME = "OM_Android_Auto_Display";

  @NonNull
  private final LifecycleOwner mLifecycleOwner;

  @NonNull
  private final LocationHelper mLocationHelper;

  @NonNull
  private final MapRenderingListener mMapRenderingListener;

  @NonNull
  private final DisplayManager mDisplayManager;

  @Nullable
  private VirtualDisplay mVirtualDisplay;
  @Nullable
  private CarPresentation mPresentation;

  public SurfaceCallback(@NonNull CarContext carContext, @NonNull LifecycleOwner lifecycleOwner,
                         @NonNull LocationHelper locationHelper, @NonNull MapRenderingListener mapRenderingListener,
                         @NonNull DisplayManager displayManager)
  {
    super(carContext);
    mLifecycleOwner = lifecycleOwner;
    mLocationHelper = locationHelper;
    mMapRenderingListener = mapRenderingListener;
    mDisplayManager = displayManager;
  }

  @Override
  public void onSurfaceAvailable(@NonNull SurfaceContainer surfaceContainer)
  {
    Logger.d(TAG, "Surface available " + surfaceContainer);

    if (mVirtualDisplay == null)
    {
      final android.hardware.display.DisplayManager displayManager =
          ContextCompat.getSystemService(mCarContext, android.hardware.display.DisplayManager.class);
      mVirtualDisplay =
          Objects.requireNonNull(displayManager)
              .createVirtualDisplay(VIRTUAL_DISPLAY_NAME, surfaceContainer.getWidth(), surfaceContainer.getHeight(),
                                    surfaceContainer.getDpi(), surfaceContainer.getSurface(), 0);
    }
    else
    {
      mVirtualDisplay.setSurface(surfaceContainer.getSurface());
      mVirtualDisplay.resize(surfaceContainer.getWidth(), surfaceContainer.getHeight(), surfaceContainer.getDpi());
    }

    if (mPresentation == null)
      createPresentation();
    startPresenting();
  }

  @Override
  public void onVisibleAreaChanged(@NonNull Rect visibleArea)
  {
    super.onVisibleAreaChanged(visibleArea);
    if (mPresentation != null)
      mPresentation.setVisibleArea(visibleArea);
  }

  @Override
  public void onSurfaceDestroyed(@NonNull SurfaceContainer surfaceContainer)
  {
    Logger.d(TAG, "Surface destroyed");

    stopPresenting();
    if (mVirtualDisplay != null)
      mVirtualDisplay.setSurface(null);
  }

  void destroy()
  {
    clearPresentation();

    if (mVirtualDisplay != null)
    {
      mVirtualDisplay.release();
      mVirtualDisplay = null;
    }
  }

  void stopPresenting()
  {
    if (mPresentation != null)
      mPresentation.stopPresenting();
  }

  void startPresenting()
  {
    if (mPresentation != null && mDisplayManager.isCarDisplayUsed())
      mPresentation.startPresenting();
  }

  void setSpeedLimit(int speedLimit, boolean speedLimitExceeded)
  {
    if (mPresentation != null)
      mPresentation.setSpeedLimit(speedLimit, speedLimitExceeded);
  }

  private void createPresentation()
  {
    if (mVirtualDisplay == null)
      return;

    mPresentation = new CarPresentation(mCarContext, mVirtualDisplay.getDisplay(), mLifecycleOwner, mLocationHelper,
                                        mMapRenderingListener);
    mPresentation.create();
    mPresentation.setVisibleArea(mVisibleArea);
  }

  private void clearPresentation()
  {
    if (mPresentation != null)
    {
      mPresentation.destroy();
      mPresentation = null;
    }
  }
}
