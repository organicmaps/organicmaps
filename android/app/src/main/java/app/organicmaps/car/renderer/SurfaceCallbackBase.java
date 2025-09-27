package app.organicmaps.car.renderer;

import android.graphics.Rect;
import androidx.annotation.NonNull;
import androidx.car.app.CarContext;
import androidx.car.app.SurfaceCallback;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.Map;
import app.organicmaps.sdk.util.concurrency.UiThread;
import app.organicmaps.sdk.util.log.Logger;

abstract class SurfaceCallbackBase implements SurfaceCallback
{
  @NonNull
  private final String TAG;

  @NonNull
  protected final CarContext mCarContext;

  @NonNull
  protected Rect mVisibleArea = new Rect();

  public SurfaceCallbackBase(@NonNull CarContext carContext)
  {
    TAG = getClass().getSimpleName();
    mCarContext = carContext;
  }

  @Override
  public void onVisibleAreaChanged(@NonNull Rect visibleArea)
  {
    Logger.d(TAG, "Visible area changed. visibleArea: " + visibleArea);
    mVisibleArea = visibleArea;

    if (!mVisibleArea.isEmpty())
      UiThread.runLater(()
                            -> Framework.nativeSetVisibleRect(mVisibleArea.left, mVisibleArea.top, mVisibleArea.right,
                                                              mVisibleArea.bottom));
  }

  @Override
  public void onStableAreaChanged(@NonNull Rect stableArea)
  {
    Logger.d(TAG, "Stable area changed. stableArea: " + stableArea);

    if (!mVisibleArea.isEmpty())
      UiThread.runLater(()
                            -> Framework.nativeSetVisibleRect(mVisibleArea.left, mVisibleArea.top, mVisibleArea.right,
                                                              mVisibleArea.bottom));
    else if (!stableArea.isEmpty())
      UiThread.runLater(
          () -> Framework.nativeSetVisibleRect(stableArea.left, stableArea.top, stableArea.right, stableArea.bottom));
  }

  @Override
  public void onScroll(float distanceX, float distanceY)
  {
    Logger.d(TAG, "distanceX: " + distanceX + ", distanceY: " + distanceY);
    Map.onScroll(distanceX, distanceY);
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
