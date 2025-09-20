package app.organicmaps.car.renderer;

import android.graphics.Rect;
import android.view.Surface;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.car.app.CarContext;
import androidx.car.app.SurfaceContainer;
import app.organicmaps.sdk.Map;
import app.organicmaps.sdk.location.LocationHelper;
import app.organicmaps.sdk.util.log.Logger;

class SurfaceCallbackLegacy extends SurfaceCallbackBase
{
  private static final String TAG = SurfaceCallbackLegacy.class.getSimpleName();

  @NonNull
  private final Map mMap;

  @NonNull
  private final LocationHelper mLocationHelper;

  @Nullable
  private Surface mSurface = null;

  public SurfaceCallbackLegacy(@NonNull CarContext carContext, @NonNull Map map, @NonNull LocationHelper locationHelper)
  {
    super(carContext);
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
}
