package app.organicmaps.sdk.car.renderer;

import android.app.Presentation;
import android.content.Context;
import android.graphics.Rect;
import android.os.Bundle;
import android.view.Display;
import android.view.Gravity;
import android.view.SurfaceHolder;
import android.widget.FrameLayout;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.lifecycle.LifecycleOwner;
import app.organicmaps.sdk.MapController;
import app.organicmaps.sdk.MapRenderingListener;
import app.organicmaps.sdk.MapView;
import app.organicmaps.sdk.car.R;
import app.organicmaps.sdk.location.LocationHelper;
import app.organicmaps.sdk.widgets.speedlimit.SpeedLimitView;

final class CarPresentation extends Presentation
{
  @NonNull
  private final LifecycleOwner mLifecycleOwner;

  @NonNull
  private final LocationHelper mLocationHelper;

  @NonNull
  private final MapRenderingListener mMapRenderingListener;

  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private SpeedLimitView mSpeedLimitView;

  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private FrameLayout mVisibleAreaContainer;

  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private MapController mMapController;

  private int mSpeedLimit = 0;
  private boolean mSpeedLimitExceeded = false;
  @NonNull
  private final Rect mVisibleArea = new Rect();

  public CarPresentation(@NonNull Context outerContext, @NonNull Display display,
                         @NonNull LifecycleOwner lifecycleOwner, @NonNull LocationHelper locationHelper,
                         @NonNull MapRenderingListener mapRenderingListener)
  {
    super(outerContext, display);
    mLifecycleOwner = lifecycleOwner;
    mLocationHelper = locationHelper;
    mMapRenderingListener = mapRenderingListener;
  }

  @Override
  protected void onCreate(@Nullable Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.car_layout);

    final MapView mapView = findViewById(R.id.map_view);
    mSpeedLimitView = findViewById(R.id.speed_limit_view);
    mVisibleAreaContainer = findViewById(R.id.visible_area_container);
    mSpeedLimitView.setSpeedLimit(mSpeedLimit, mSpeedLimitExceeded);
    applyVisibleArea();

    mMapController = new MapController(mapView, mLocationHelper, mMapRenderingListener, null, false);
    mapView.getHolder().addCallback(new SurfaceHolder.Callback() {
      @Override
      public void surfaceChanged(@NonNull SurfaceHolder holder, int format, int width, int height)
      {
        mMapController.updateMyPositionRoutingOffset(0);
      }

      @Override
      public void surfaceCreated(@NonNull SurfaceHolder holder)
      {
        mMapController.updateMyPositionRoutingOffset(0);
      }

      @Override
      public void surfaceDestroyed(@NonNull SurfaceHolder holder)
      {}
    });
  }

  void startPresenting()
  {
    if (isShowing())
      return;

    show();
    mMapController.onStart(mLifecycleOwner);
    mMapController.updateMyPositionRoutingOffset(0);
  }

  void stopPresenting()
  {
    if (!isShowing())
      return;

    mMapController.onPause(mLifecycleOwner);
    mMapController.onStop(mLifecycleOwner);
    dismiss();
  }

  void setSpeedLimit(int speedLimit, boolean speedLimitExceeded)
  {
    mSpeedLimit = speedLimit;
    mSpeedLimitExceeded = speedLimitExceeded;
    mSpeedLimitView.setSpeedLimit(speedLimit, speedLimitExceeded);
  }

  void setVisibleArea(@NonNull Rect visibleArea)
  {
    mVisibleArea.set(visibleArea);
    applyVisibleArea();
  }

  private void applyVisibleArea()
  {
    if (mVisibleArea.isEmpty())
      return;

    final FrameLayout.LayoutParams layoutParams =
        new FrameLayout.LayoutParams(mVisibleArea.right - mVisibleArea.left, mVisibleArea.bottom - mVisibleArea.top);
    layoutParams.leftMargin = mVisibleArea.left;
    layoutParams.topMargin = mVisibleArea.top;
    layoutParams.gravity = Gravity.NO_GRAVITY;
    mVisibleAreaContainer.setLayoutParams(layoutParams);
  }

  void destroy()
  {
    if (isShowing())
    {
      mMapController.onPause(mLifecycleOwner);
      mMapController.onStop(mLifecycleOwner);
    }
    mMapController.onDestroy(mLifecycleOwner);
    dismiss();
  }
}
