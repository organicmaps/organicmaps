package app.organicmaps.car.renderer;

import androidx.annotation.CallSuper;
import androidx.annotation.NonNull;
import androidx.car.app.AppManager;
import androidx.car.app.CarContext;
import androidx.car.app.SurfaceCallback;
import androidx.lifecycle.DefaultLifecycleObserver;
import androidx.lifecycle.LifecycleOwner;
import app.organicmaps.sdk.Map;
import app.organicmaps.sdk.MapRenderingListener;
import app.organicmaps.sdk.display.DisplayManager;
import app.organicmaps.sdk.location.LocationHelper;
import app.organicmaps.sdk.settings.UnitLocale;
import app.organicmaps.sdk.util.log.Logger;

public abstract class Renderer implements DefaultLifecycleObserver
{
  private static final String TAG = Renderer.class.getSimpleName();

  private SurfaceCallback mSurfaceCallback;

  private boolean mIsRunning;

  @NonNull
  protected final CarContext mCarContext;

  @NonNull
  protected final DisplayManager mDisplayManager;

  @NonNull
  protected final LocationHelper mLocationHelper;

  @NonNull
  protected final LifecycleOwner mLifecycleOwner;

  @NonNull
  private final MapRenderingListener mMapRenderingListener = new MapRenderingListener() {
    @Override
    public void onRenderingCreated()
    {
      UnitLocale.initializeCurrentUnits();
    }
  };

  public Renderer(@NonNull CarContext carContext, @NonNull DisplayManager displayManager,
                  @NonNull LocationHelper locationHelper, @NonNull LifecycleOwner lifecycleOwner)
  {
    Logger.d(TAG, "SurfaceRenderer()");
    mIsRunning = true;
    mCarContext = carContext;
    mDisplayManager = displayManager;
    mLocationHelper = locationHelper;
    mLifecycleOwner = lifecycleOwner;
    mLifecycleOwner.getLifecycle().addObserver(this);
  }

  protected void setSurfaceCallback(@NonNull SurfaceCallback surfaceCallback)
  {
    mSurfaceCallback = surfaceCallback;
  }

  public boolean isRenderingActive()
  {
    return mIsRunning;
  }

  protected MapRenderingListener getMapRenderingListener()
  {
    return mMapRenderingListener;
  }

  @CallSuper
  @Override
  public void onCreate(@NonNull LifecycleOwner owner)
  {
    Logger.d(TAG);
    if (mSurfaceCallback == null)
      throw new IllegalStateException("SurfaceCallback must be set before onCreate()");
    mCarContext.getCarService(AppManager.class).setSurfaceCallback(mSurfaceCallback);
  }

  @CallSuper
  @Override
  public void onDestroy(@NonNull LifecycleOwner owner)
  {
    Logger.d(TAG);
    mCarContext.getCarService(AppManager.class).setSurfaceCallback(null);
  }

  @CallSuper
  public void enable()
  {
    if (isRenderingActive())
    {
      Logger.d(TAG, "Already enabled");
      return;
    }

    if (mSurfaceCallback == null)
      throw new IllegalStateException("SurfaceCallback must be set before enable()");
    mCarContext.getCarService(AppManager.class).setSurfaceCallback(mSurfaceCallback);
    mIsRunning = true;
  }

  @CallSuper
  public void disable()
  {
    if (!isRenderingActive())
    {
      Logger.d(TAG, "Already disabled");
      return;
    }

    mCarContext.getCarService(AppManager.class).setSurfaceCallback(null);
    mIsRunning = false;
  }

  public void onZoomIn()
  {
    Map.zoomIn();
  }

  public void onZoomOut()
  {
    Map.zoomOut();
  }
}
