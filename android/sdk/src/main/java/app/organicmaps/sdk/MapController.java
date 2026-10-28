package app.organicmaps.sdk;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.lifecycle.DefaultLifecycleObserver;
import androidx.lifecycle.LifecycleOwner;
import app.organicmaps.sdk.location.LocationHelper;
import app.organicmaps.sdk.util.log.Logger;

/**
 * Connects one {@link MapView} to its host lifecycle on the main thread.
 * Create after map loading completes and before attaching the view to a window, then register it with
 * {@code lifecycle.addObserver(controller)}. Rendering callbacks are released when the lifecycle is destroyed.
 * Location permissions and polling remain the host application's responsibility.
 */
public class MapController implements DefaultLifecycleObserver
{
  private static final String TAG_PREFIX = MapController.class.getSimpleName();
  @NonNull
  private final String TAG;

  @NonNull
  private final MapView mMapView;
  @NonNull
  private final Map mMap;

  @Nullable
  private Runnable mOnSurfaceDestroyedListener = null;

  /**
   * Binds a map view to the initialized framework and its services.
   *
   * @param mapView view belonging to this lifecycle owner
   * @param locationHelper location helper from the application's {@link OrganicMaps} instance
   * @param mapRenderingListener rendering callbacks; retained until lifecycle destruction
   * @param callbackUnsupported optional callback when a graphics surface cannot be created or attached
   * @param launchByDeepLink whether initial rendering is being opened by a map API request
   */
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
    TAG = TAG_PREFIX + "[" + mMap.getDisplayType() + "]";
  }

  public MapView getView()
  {
    return mMapView;
  }

  /**
   * Returns whether the native engine exists and this map has a graphics context.
   * This does not report completion of asynchronous map loading or guarantee that a frame is visible.
   */
  public boolean isRenderingActive()
  {
    return Map.isEngineCreated() && mMap.isContextCreated();
  }

  /**
   * Moves the compass away from overlays and window insets.
   *
   * @param offsetX inset from the right edge, in pixels; negative keeps the previous value
   * @param offsetY inset from the top edge, in pixels; negative keeps the previous value
   */
  public void updateCompassOffset(int offsetX, int offsetY)
  {
    mMap.updateCompassOffset(mMapView.getContext(), offsetX, offsetY, true);
  }

  /**
   * Moves the ruler and attribution away from overlays and window insets.
   *
   * @param offsetX inset from the left edge, in pixels; negative keeps the previous value
   * @param offsetY inset from the bottom edge, in pixels; negative keeps the previous value
   */
  public void updateBottomWidgetsOffset(int offsetX, int offsetY)
  {
    mMap.updateBottomWidgetsOffset(mMapView.getContext(), offsetX, offsetY);
  }

  /**
   * Sets the navigation position-arrow offset from the bottom edge, in pixels.
   */
  public void updateMyPositionRoutingOffset(int offsetY)
  {
    mMap.updateMyPositionRoutingOffset(offsetY);
  }

  /**
   * Sets a one-shot callback run by {@link #onDestroy}; a later call replaces the previous callback.
   * This follows lifecycle destruction and does not wait for a {@code SurfaceHolder} destruction event.
   */
  public void setOnDestroyListener(@NonNull Runnable task)
  {
    mOnSurfaceDestroyedListener = task;
  }

  /**
   * Registers the native rendering-initialization listener when the host starts.
   */
  @Override
  public void onStart(@NonNull LifecycleOwner owner)
  {
    Logger.d(TAG);
    mMap.onStart();
  }

  /**
   * Resumes rendering on an attached surface when the host resumes.
   */
  @Override
  public void onResume(@NonNull LifecycleOwner owner)
  {
    Logger.d(TAG);
    mMap.onResume();
  }

  /**
   * Pauses rendering on an attached surface when the host pauses.
   */
  @Override
  public void onPause(@NonNull LifecycleOwner owner)
  {
    Logger.d(TAG);
    mMap.onPause();
  }

  /**
   * Unregisters the native rendering-initialization listener when the host stops.
   */
  @Override
  public void onStop(@NonNull LifecycleOwner owner)
  {
    Logger.d(TAG);
    mMap.onStop();
  }

  /**
   * Releases rendering callbacks and runs the optional destruction callback once.
   * The process-wide native framework remains available for the next activity.
   */
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
