package app.organicmaps.car.renderer;

import android.app.Presentation;
import android.graphics.Rect;
import android.hardware.display.DisplayManager;
import android.hardware.display.VirtualDisplay;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.SurfaceHolder;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewParent;
import android.widget.FrameLayout;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RequiresApi;
import androidx.car.app.CarContext;
import androidx.car.app.SurfaceContainer;
import app.organicmaps.sdk.MapController;
import app.organicmaps.sdk.util.log.Logger;
import app.organicmaps.widget.SpeedLimitView;

@RequiresApi(23)
class SurfaceCallback extends SurfaceCallbackBase
{
  private static final String TAG = SurfaceCallback.class.getSimpleName();

  private static final int SPEED_LIMIT_VIEW_SIZE_DP = 80;

  private static final String VIRTUAL_DISPLAY_NAME = "OM_Android_Auto_Display";

  @NonNull
  private final MapController mMapController;
  @Nullable
  private FrameLayout mSpeedLimitContainer;
  @Nullable
  private SpeedLimitView mSpeedLimitView;

  private final int mSpeedLimitViewSize;

  @Nullable
  private VirtualDisplay mVirtualDisplay;
  @Nullable
  private Presentation mPresentation;

  public SurfaceCallback(@NonNull CarContext carContext, @NonNull MapController mapController)
  {
    super(carContext);
    mMapController = mapController;
    mMapController.getView().getHolder().addCallback(new SurfaceHolder.Callback() {
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
    mSpeedLimitViewSize = (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, SPEED_LIMIT_VIEW_SIZE_DP,
                                                          mCarContext.getResources().getDisplayMetrics());
    initSpeedLimitView();
  }

  @Override
  public void onSurfaceAvailable(@NonNull SurfaceContainer surfaceContainer)
  {
    Logger.d(TAG, "Surface available " + surfaceContainer);

    mVirtualDisplay =
        mCarContext.getSystemService(DisplayManager.class)
            .createVirtualDisplay(VIRTUAL_DISPLAY_NAME, surfaceContainer.getWidth(), surfaceContainer.getHeight(),
                                  surfaceContainer.getDpi(), surfaceContainer.getSurface(),
                                  DisplayManager.VIRTUAL_DISPLAY_FLAG_OWN_CONTENT_ONLY);
    mPresentation = new Presentation(mCarContext, mVirtualDisplay.getDisplay());

    mPresentation.setContentView(prepareViewForPresentation(mMapController.getView()));
    mPresentation.show();
  }

  @Override
  public void onVisibleAreaChanged(@NonNull Rect visibleArea)
  {
    super.onVisibleAreaChanged(visibleArea);

    assert mSpeedLimitContainer != null : "mSpeedLimitContainer must be initialized";
    mSpeedLimitContainer.setLayoutParams(getSpeedLimitContainerLayoutParams());
  }

  @Override
  public void onSurfaceDestroyed(@NonNull SurfaceContainer surfaceContainer)
  {
    Logger.d(TAG, "Surface destroyed");
    if (mPresentation != null)
      mPresentation.dismiss();
    if (mVirtualDisplay != null)
      mVirtualDisplay.release();
  }

  @NonNull
  SpeedLimitView getSpeedLimitView()
  {
    assert mSpeedLimitView != null : "mSpeedLimitContainer must be initialized";
    return mSpeedLimitView;
  }

  void stopPresenting()
  {
    if (mPresentation != null)
      mPresentation.dismiss();
  }

  void startPresenting()
  {
    if (mPresentation != null)
      mPresentation.show();
  }

  @NonNull
  private View prepareViewForPresentation(@NonNull View view)
  {
    final ViewParent parent = view.getParent();
    if (parent instanceof ViewGroup)
      ((ViewGroup) parent).removeView(view);

    final FrameLayout container = new FrameLayout(mCarContext);
    container.addView(
        view, new FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
    initSpeedLimitView();
    container.addView(mSpeedLimitContainer);

    return container;
  }

  private void initSpeedLimitView()
  {
    mSpeedLimitContainer = new FrameLayout(mCarContext);
    mSpeedLimitContainer.setLayoutParams(getSpeedLimitContainerLayoutParams());

    final boolean restoreOldState = mSpeedLimitView != null;
    final boolean isAlert = restoreOldState && mSpeedLimitView.isAlert();
    final int speedLimit = restoreOldState ? mSpeedLimitView.getSpeedLimit() : 0;

    mSpeedLimitView = new SpeedLimitView(mCarContext);
    if (restoreOldState)
      mSpeedLimitView.setSpeedLimit(speedLimit, isAlert);

    final FrameLayout.LayoutParams speedLimitLayoutParams =
        new FrameLayout.LayoutParams(mSpeedLimitViewSize, mSpeedLimitViewSize);
    speedLimitLayoutParams.gravity = Gravity.END | Gravity.BOTTOM;
    mSpeedLimitContainer.addView(mSpeedLimitView, speedLimitLayoutParams);
  }

  @NonNull
  private ViewGroup.LayoutParams getSpeedLimitContainerLayoutParams()
  {
    final FrameLayout.LayoutParams layoutParams =
        new FrameLayout.LayoutParams(mVisibleArea.right - mVisibleArea.left, // width
                                     mVisibleArea.bottom - mVisibleArea.top // height
        );
    layoutParams.leftMargin = mVisibleArea.left;
    layoutParams.topMargin = mVisibleArea.top;
    layoutParams.gravity = Gravity.NO_GRAVITY;
    return layoutParams;
  }
}
