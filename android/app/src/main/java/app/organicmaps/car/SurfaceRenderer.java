package app.organicmaps.car;

import static app.organicmaps.sdk.display.DisplayType.Car;

import android.graphics.Rect;
import android.view.Surface;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.car.app.AppManager;
import androidx.car.app.CarContext;
import androidx.car.app.CarToast;
import androidx.car.app.SurfaceCallback;
import androidx.car.app.SurfaceContainer;
import androidx.lifecycle.DefaultLifecycleObserver;
import androidx.lifecycle.Lifecycle;
import androidx.lifecycle.LifecycleOwner;
import app.organicmaps.MwmApplication;
import app.organicmaps.R;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.Map;
import app.organicmaps.sdk.MapRenderingListener;
import app.organicmaps.sdk.settings.UnitLocale;
import app.organicmaps.sdk.util.concurrency.UiThread;
import app.organicmaps.sdk.util.log.Logger;

public class SurfaceRenderer implements DefaultLifecycleObserver, SurfaceCallback, MapRenderingListener
{
  private static final String TAG = SurfaceRenderer.class.getSimpleName();

  private final CarContext mCarContext;
  private final Map mMap = new Map(Car);

  @NonNull
  private Rect mVisibleArea = new Rect();

  @Nullable
  private Surface mSurface = null;

  private boolean mIsRunning;

  public SurfaceRenderer(@NonNull CarContext carContext, @NonNull Lifecycle lifecycle)
  {
    Logger.d(TAG, "SurfaceRenderer()");
    mCarContext = carContext;
    mIsRunning = true;
    lifecycle.addObserver(this);
    mMap.setMapRenderingListener(this);
  }

  @Override
  public void onSurfaceAvailable(@NonNull SurfaceContainer surfaceContainer)
  {
    Logger.d(TAG, "Surface available " + surfaceContainer);

    if (mSurface != null)
      mSurface.release();
    mSurface = surfaceContainer.getSurface();

    mMap.onSurfaceCreated(mCarContext, mSurface,
                          new Rect(0, 0, surfaceContainer.getWidth(), surfaceContainer.getHeight()),
                          surfaceContainer.getDpi());
    mMap.updateBottomWidgetsOffset(mCarContext, -1, -1);
  }

  @Override
  public void onVisibleAreaChanged(@NonNull Rect visibleArea)
  {
    Logger.d(TAG, "Visible area changed. visibleArea: " + visibleArea);
    mVisibleArea = visibleArea;

    if (!mVisibleArea.isEmpty())
      Framework.nativeSetVisibleRect(mVisibleArea.left, mVisibleArea.top, mVisibleArea.right, mVisibleArea.bottom);
  }

  @Override
  public void onStableAreaChanged(@NonNull Rect stableArea)
  {
    Logger.d(TAG, "Stable area changed. stableArea: " + stableArea);

    if (!stableArea.isEmpty())
      Framework.nativeSetVisibleRect(stableArea.left, stableArea.top, stableArea.right, stableArea.bottom);
    else if (!mVisibleArea.isEmpty())
      Framework.nativeSetVisibleRect(mVisibleArea.left, mVisibleArea.top, mVisibleArea.right, mVisibleArea.bottom);
  }

  @Override
  public void onSurfaceDestroyed(@NonNull SurfaceContainer surfaceContainer)
  {
    Logger.d(TAG, "Surface destroyed");
    if (mSurface != null)
    {
      mSurface.release();
      mSurface = null;
    }
    mMap.onSurfaceDestroyed(false, true);
  }

  @Override
  public void onCreate(@NonNull LifecycleOwner owner)
  {
    Logger.d(TAG);
    mCarContext.getCarService(AppManager.class).setSurfaceCallback(this);
    mMap.onCreate(false);
  }

  @Override
  public void onStart(@NonNull LifecycleOwner owner)
  {
    Logger.d(TAG);
    mMap.onStart();
    mMap.setCallbackUnsupported(this::reportUnsupported);
  }

  @Override
  public void onResume(@NonNull LifecycleOwner owner)
  {
    Logger.d(TAG);
    mMap.onResume();
    if (MwmApplication.from(mCarContext).getDisplayManager().isCarDisplayUsed())
      UiThread.runLater(() -> mMap.updateMyPositionRoutingOffset(0));
  }

  @Override
  public void onPause(@NonNull LifecycleOwner owner)
  {
    Logger.d(TAG);
    mMap.onPause(mCarContext);
  }

  @Override
  public void onStop(@NonNull LifecycleOwner owner)
  {
    Logger.d(TAG);
    mMap.onStop();
    mMap.setCallbackUnsupported(null);
  }

  @Override
  public void onScroll(float distanceX, float distanceY)
  {
    Logger.d(TAG, "distanceX: " + distanceX + ", distanceY: " + distanceY);
    mMap.onScroll(distanceX, distanceY);
  }

  @Override
  public void onFling(float velocityX, float velocityY)
  {
    Logger.d(TAG, "velocityX: " + velocityX + ", velocityY: " + velocityY);
  }

  public void onZoomIn()
  {
    Map.zoomIn();
  }

  public void onZoomOut()
  {
    Map.zoomOut();
  }

  @Override
  public void onScale(float focusX, float focusY, float scaleFactor)
  {
    Logger.d(TAG, "focusX: " + focusX + ", focusY: " + focusY + ", scaleFactor: " + scaleFactor);
    float x = focusX;
    float y = focusY;

    if (!mVisibleArea.isEmpty())
    {
      // If a focal point value is negative, use the center point of the visible area.
      if (x < 0)
        x = mVisibleArea.centerX();
      if (y < 0)
        y = mVisibleArea.centerY();
    }

    final boolean animated = Float.compare(scaleFactor, 2f) == 0;

    Map.onScale(scaleFactor, x, y, animated);
  }

  @Override
  public void onClick(float x, float y)
  {
    Logger.d(TAG, "x: " + x + ", y: " + y);
    Map.onClick(x, y);
  }

  public void disable()
  {
    if (!mIsRunning)
    {
      Logger.d(TAG, "Already disabled");
      return;
    }

    mCarContext.getCarService(AppManager.class).setSurfaceCallback(null);
    mMap.onSurfaceDestroyed(false, true);
    mMap.onStop();
    mMap.setCallbackUnsupported(null);
    mMap.setMapRenderingListener(null);

    mIsRunning = false;
  }

  public void enable()
  {
    if (mIsRunning)
    {
      Logger.d(TAG, "Already enabled");
      return;
    }

    mCarContext.getCarService(AppManager.class).setSurfaceCallback(this);
    mMap.onStart();
    mMap.setCallbackUnsupported(this::reportUnsupported);
    mMap.setMapRenderingListener(this);
    UiThread.runLater(() -> mMap.updateMyPositionRoutingOffset(0));

    mIsRunning = true;
  }

  public boolean isRenderingActive()
  {
    return mIsRunning;
  }

  private void reportUnsupported()
  {
    String message = mCarContext.getString(R.string.unsupported_phone);
    Logger.e(TAG, message);
    CarToast.makeText(mCarContext, message, CarToast.LENGTH_LONG).show();
  }

  @Override
  public void onRenderingCreated()
  {
    UnitLocale.initializeCurrentUnits();
  }
}
