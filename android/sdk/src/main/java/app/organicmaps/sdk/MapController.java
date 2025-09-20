package app.organicmaps.sdk;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.lifecycle.DefaultLifecycleObserver;
import androidx.lifecycle.LifecycleOwner;
import app.organicmaps.sdk.location.LocationHelper;
import app.organicmaps.sdk.util.log.Logger;

public class MapController implements DefaultLifecycleObserver
{
  private static final String TAG_PEFRIX = MapController.class.getSimpleName();
  @NonNull
  private final String TAG;

  @NonNull
  private final MapView mMapView;
  @NonNull
  private final Map mMap;

  @Nullable
  private Runnable mOnSurfaceDestroyedListener = null;

  public MapController(@NonNull MapView mapView, @NonNull LocationHelper locationHelper,
                       @NonNull MapRenderingListener mapRenderingListener,
                       @Nullable Map.CallbackUnsupported callbackUnsupported, boolean launchByDeepLink)
  {
    mMapView = mapView;
    mMap = mMapView.getMap();
    mMap.onCreate(launchByDeepLink);
    mMap.setLocationHelper(locationHelper);
    mMap.setMapRenderingListener(mapRenderingListener);
    mMap.setCallbackUnsupported(callbackUnsupported);
    TAG = TAG_PEFRIX + "[" + mMap.getDisplayType() + "]";
  }

  public MapView getView()
  {
    return mMapView;
  }

  public boolean isRenderingActive()
  {
    return Map.isEngineCreated() && mMap.isContextCreated();
  }

  public void updateCompassOffset(int offsetX, int offsetY)
  {
    mMap.updateCompassOffset(mMapView.getContext(), offsetX, offsetY, true);
  }

  public void updateBottomWidgetsOffset(int offsetX, int offsetY)
  {
    mMap.updateBottomWidgetsOffset(mMapView.getContext(), offsetX, offsetY);
  }

  public void updateMyPositionRoutingOffset(int offsetY)
  {
    mMap.updateMyPositionRoutingOffset(offsetY);
  }

  public void setOnDestroyListener(@NonNull Runnable task)
  {
    mOnSurfaceDestroyedListener = task;
  }

  @Override
  public void onStart(@NonNull LifecycleOwner owner)
  {
    Logger.d(TAG);
    mMap.onStart();
  }

  @Override
  public void onResume(@NonNull LifecycleOwner owner)
  {
    Logger.d(TAG);
    mMap.onResume();
  }

  @Override
  public void onPause(@NonNull LifecycleOwner owner)
  {
    Logger.d(TAG);
    mMap.onPause();
  }

  @Override
  public void onStop(@NonNull LifecycleOwner owner)
  {
    Logger.d(TAG);
    mMap.onStop();
  }

  @Override
  public void onDestroy(@NonNull LifecycleOwner owner)
  {
    Logger.d(TAG);
    mMap.setMapRenderingListener(null);
    mMap.setCallbackUnsupported(null);
    if (mOnSurfaceDestroyedListener != null)
    {
      mOnSurfaceDestroyedListener.run();
      mOnSurfaceDestroyedListener = null;
    }
  }
}
