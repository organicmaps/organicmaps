package app.organicmaps.car;

import android.graphics.Rect;

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

import app.organicmaps.Framework;
import app.organicmaps.Map;
import app.organicmaps.Map.MapType;
import app.organicmaps.R;
import app.organicmaps.util.log.Logger;

public class SurfaceRenderer implements DefaultLifecycleObserver, SurfaceCallback
{
  private static final String TAG = SurfaceRenderer.class.getSimpleName();

  private final CarContext mCarContext;
  private final Map mMap = new Map(MapType.AndroidAuto);

  @Nullable
  private Rect mVisibleArea;
  @Nullable
  private Rect mStableArea;

  public SurfaceRenderer(@NonNull CarContext carContext, @NonNull Lifecycle lifecycle)
  {
    Logger.d(TAG, "SurfaceRenderer()");
    mCarContext = carContext;
    lifecycle.addObserver(this);
  }

  @Override
  public void onSurfaceAvailable(@NonNull SurfaceContainer surfaceContainer)
  {
    Logger.d(TAG, "Surface available " + surfaceContainer);
    mMap.onSurfaceCreated(
        mCarContext,
        surfaceContainer.getSurface(),
        new Rect(0, 0, surfaceContainer.getWidth(), surfaceContainer.getHeight()),
        surfaceContainer.getDpi()
    );
  }

  @Override
  public void onVisibleAreaChanged(@NonNull Rect visibleArea)
  {
    Logger.d(TAG, "Visible area changed. visibleArea: " + visibleArea);
    mVisibleArea = visibleArea;
  }

  @Override
  public void onStableAreaChanged(@NonNull Rect stableArea)
  {
    Logger.d(TAG, "Stable area changed. stableArea: " + stableArea);
    mStableArea = stableArea;
    Framework.nativeSetVisibleRect(mStableArea.left, mStableArea.top, mStableArea.right, mStableArea.bottom);
  }

  @Override
  public void onSurfaceDestroyed(@NonNull SurfaceContainer surfaceContainer)
  {
    Logger.d(TAG, "Surface destroyed");
    mMap.onSurfaceDestroyed(false, true);
  }

  @Override
  public void onCreate(@NonNull LifecycleOwner owner)
  {
    Logger.d(TAG, "onCreate");
    mCarContext.getCarService(AppManager.class).setSurfaceCallback(this);

    // TODO: Properly process deep links from other apps on AA.
    boolean launchByDeepLink = false;
    mMap.onCreate(launchByDeepLink);
  }

  @Override
  public void onStart(@NonNull LifecycleOwner owner)
  {
    Logger.d(TAG, "onStart");
    mMap.onStart();
    mMap.setCallbackUnsupported(this::reportUnsupported);
  }

  @Override
  public void onStop(@NonNull LifecycleOwner owner)
  {
    Logger.d(TAG, "onStop");
    mMap.onStop();
    mMap.setCallbackUnsupported(null);
  }

  @Override
  public void onPause(@NonNull LifecycleOwner owner)
  {
    Logger.d(TAG, "onPause");
    mMap.onPause(mCarContext);
  }

  @Override
  public void onResume(@NonNull LifecycleOwner owner)
  {
    Logger.d(TAG, "onResume");
    mMap.onResume();
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

    Rect visibleArea = mVisibleArea;
    if (visibleArea != null)
    {
      // If a focal point value is negative, use the center point of the visible area.
      if (x < 0)
        x = visibleArea.centerX();
      if (y < 0)
        y = visibleArea.centerY();
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

  private void reportUnsupported()
  {
    String message = mCarContext.getString(R.string.unsupported_phone);
    Logger.e(TAG, message);
    CarToast.makeText(mCarContext, message, CarToast.LENGTH_LONG).show();
  }
}
