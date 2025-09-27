package app.organicmaps.car.renderer;

import android.app.Presentation;
import android.hardware.display.DisplayManager;
import android.hardware.display.VirtualDisplay;
import android.view.SurfaceHolder;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewParent;
import android.widget.FrameLayout;
import androidx.annotation.NonNull;
import androidx.annotation.RequiresApi;
import androidx.car.app.CarContext;
import androidx.car.app.SurfaceContainer;
import app.organicmaps.sdk.MapController;
import app.organicmaps.sdk.util.log.Logger;

@RequiresApi(23)
class SurfaceCallback extends SurfaceCallbackBase
{
  private static final String TAG = SurfaceCallback.class.getSimpleName();

  private static final String VIRTUAL_DISPLAY_NAME = "OM_Android_Auto_Display";
  private static final int VIRTUAL_DISPLAY_FLAGS =
      DisplayManager.VIRTUAL_DISPLAY_FLAG_PRESENTATION | DisplayManager.VIRTUAL_DISPLAY_FLAG_OWN_CONTENT_ONLY;

  @NonNull
  private final MapController mMapController;

  private VirtualDisplay mVirtualDisplay;
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
  }

  @Override
  public void onSurfaceAvailable(@NonNull SurfaceContainer surfaceContainer)
  {
    Logger.d(TAG, "Surface available " + surfaceContainer);

    mVirtualDisplay =
        mCarContext.getSystemService(DisplayManager.class)
            .createVirtualDisplay(VIRTUAL_DISPLAY_NAME, surfaceContainer.getWidth(), surfaceContainer.getHeight(),
                                  surfaceContainer.getDpi(), surfaceContainer.getSurface(), VIRTUAL_DISPLAY_FLAGS);
    mPresentation = new Presentation(mCarContext, mVirtualDisplay.getDisplay());

    mPresentation.setContentView(prepareViewForPresentation(mMapController.getView()));
    mPresentation.show();
  }

  @Override
  public void onSurfaceDestroyed(@NonNull SurfaceContainer surfaceContainer)
  {
    Logger.d(TAG, "Surface destroyed");
    mPresentation.dismiss();
    mVirtualDisplay.release();
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

    return container;
  }
}
