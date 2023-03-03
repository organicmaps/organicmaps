package app.organicmaps.car;

import android.graphics.Rect;

import androidx.annotation.NonNull;
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
import app.organicmaps.R;
import app.organicmaps.util.log.Logger;

import static app.organicmaps.display.DisplayType.Car;

public class SurfaceRenderer implements DefaultLifecycleObserver, SurfaceCallback
{
  private static final String TAG = SurfaceRenderer.class.getSimpleName();

  private final CarContext mCarContext;
  private final Map mMap = new Map(Car);

  @NonNull
  private Rect mVisibleArea = new Rect();
  @NonNull
  private Rect mStableArea = new Rect();

  private boolean mIsRunning;

  public SurfaceRenderer(@NonNull CarContext carContext, @NonNull Lifecycle lifecycle)
  {
    Logger.d(TAG, "SurfaceRenderer()");
    mCarContext = carContext;
    mIsRunning = true;
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

    if (!mVisibleArea.isEmpty())
      Framework.nativeSetVisibleRect(mVisibleArea.left, mVisibleArea.top, mVisibleArea.right, mVisibleArea.bottom);
  }

  @Override
  public void onStableAreaChanged(@NonNull Rect stableArea)
  {
    Logger.d(TAG, "Stable area changed. stableArea: " + stableArea);
    mStableArea = stableArea;

    if (!mStableArea.isEmpty())
      Framework.nativeSetVisibleRect(mStableArea.left, mStableArea.top, mStableArea.right, mStableArea.bottom);
    else if (!mVisibleArea.isEmpty())
      Framework.nativeSetVisibleRect(mVisibleArea.left, mVisibleArea.top, mVisibleArea.right, mVisibleArea.bottom);
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
//    Map.onClick(x, y);
  }

  public void disable()
  {
    if (!mIsRunning)
    {
      Logger.e(TAG, "Already disabled");
      return;
    }

    mCarContext.getCarService(AppManager.class).setSurfaceCallback(null);
    mMap.onSurfaceDestroyed(false, true);
    mMap.onStop();
    mMap.setCallbackUnsupported(null);

    mIsRunning = false;
  }

  public void enable()
  {
    if (mIsRunning)
    {
      Logger.e(TAG, "Already enabled");
      return;
    }

    mCarContext.getCarService(AppManager.class).setSurfaceCallback(this);
    mMap.onStart();
    mMap.setCallbackUnsupported(this::reportUnsupported);

    mIsRunning = true;
  }

  private void reportUnsupported()
  {
    String message = mCarContext.getString(R.string.unsupported_phone);
    Logger.e(TAG, message);
    CarToast.makeText(mCarContext, message, CarToast.LENGTH_LONG).show();
  }
}
