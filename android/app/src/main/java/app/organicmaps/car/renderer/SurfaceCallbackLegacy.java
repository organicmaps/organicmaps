package app.organicmaps.car.renderer;

import android.graphics.Rect;
import android.view.Surface;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.car.app.CarContext;
import androidx.car.app.SurfaceCallback;
import androidx.car.app.SurfaceContainer;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.Map;
import app.organicmaps.sdk.location.LocationHelper;
import app.organicmaps.sdk.util.log.Logger;

public class SurfaceCallbackLegacy implements SurfaceCallback
{
  private static final String TAG = SurfaceCallbackLegacy.class.getSimpleName();

  @NonNull
  protected final CarContext mCarContext;

  @NonNull
  private final Map mMap;

  @NonNull
  private final LocationHelper mLocationHelper;

  @Nullable
  private Surface mSurface = null;

  @NonNull
  private Rect mVisibleArea = new Rect();

  public SurfaceCallbackLegacy(@NonNull CarContext carContext, @NonNull Map map, @NonNull LocationHelper locationHelper)
  {
    mCarContext = carContext;
    mMap = map;
    mLocationHelper = locationHelper;
  }

  @Override
  public void onSurfaceAvailable(@NonNull SurfaceContainer surfaceContainer)
  {
    Logger.d(TAG, "Surface available " + surfaceContainer);

    if (mSurface != null)
      mSurface.release();
    mSurface = surfaceContainer.getSurface();

    mMap.setLocationHelper(mLocationHelper);
    mMap.onSurfaceCreated(mCarContext, mSurface,
                          new Rect(0, 0, surfaceContainer.getWidth(), surfaceContainer.getHeight()),
                          surfaceContainer.getDpi());
    mMap.updateBottomWidgetsOffset(mCarContext, -1, -1);
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
    mMap.onSurfaceDestroyed(false);
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
  public void onScroll(float distanceX, float distanceY)
  {
    Logger.d(TAG, "distanceX: " + distanceX + ", distanceY: " + distanceY);
    mMap.onScroll(distanceX, distanceY);
  }

  @Override
  public void onFling(float velocityX, float velocityY)
  {
    Logger.d(TAG, "velocityX: " + velocityX + ", velocityY: " + velocityY);
    // TODO: Implement fling in the native code.
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
}
