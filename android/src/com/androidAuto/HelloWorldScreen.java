package com.androidAuto;

import android.graphics.Canvas;
import android.graphics.Rect;
import android.util.Log;
import android.view.Surface;

import androidx.annotation.NonNull;
import androidx.car.app.AppManager;
import androidx.car.app.CarContext;
import androidx.car.app.Screen;
import androidx.car.app.SurfaceCallback;
import androidx.car.app.SurfaceContainer;
import androidx.car.app.model.Action;
import androidx.car.app.model.ActionStrip;
import androidx.car.app.model.Template;
import androidx.car.app.navigation.NavigationManager;
import androidx.car.app.navigation.model.NavigationTemplate;
import com.mapswithme.maps.BuildConfig;
import com.mapswithme.maps.MapFragment;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.settings.StoragePathManager;
import com.mapswithme.util.StorageUtils;
import com.mapswithme.util.UiUtils;

import java.io.IOException;

import static androidx.car.app.model.Action.BACK;

public class HelloWorldScreen extends Screen implements SurfaceCallback
{
  private static final String TAG = "error";
  private SurfaceCallback mSurfaceCallback;

  public HelloWorldScreen(@NonNull CarContext carContext)
  {
    super(carContext);

    carContext.getCarService(AppManager.class).setSurfaceCallback(this);
  }

  @NonNull
  @Override
  public Template onGetTemplate()
  {  Log.e("Show Up","Shows Up");
    NavigationTemplate.Builder builder = new NavigationTemplate.Builder();
    ActionStrip.Builder actionStripBuilder = new ActionStrip.Builder();
    actionStripBuilder.addAction(new Action.Builder().setTitle("Exit").setOnClickListener(this::exit).build());
    builder.setActionStrip(actionStripBuilder.build());
    NavigationManager navigationManager = getCarContext().getCarService(NavigationManager.class);
    return builder.build();
  }
  private void exit() {
    getCarContext().finishCarApp();
  }
  @Override
  public void onSurfaceAvailable(@NonNull SurfaceContainer surfaceContainer)
  {
    MwmApplication cat = MwmApplication.from(getCarContext());
    try
    {
      cat.initNativePlatform();
    }
    catch (IOException e)
    {
      e.printStackTrace();
    }
    int h = surfaceContainer.getHeight();
    int w = surfaceContainer.getWidth();
    Canvas canvas = null;

    if (surfaceContainer.getSurface() != null && MapFragment.nativeIsEngineCreated())
    {
      canvas = surfaceContainer.getSurface()
                               .lockCanvas(new Rect(Integer.MAX_VALUE, Integer.MAX_VALUE, Integer.MAX_VALUE, Integer.MAX_VALUE));
    }
    else
    {
      Log.e("NPE", "Surface Not Available");
    }
    if (canvas == null)
    {
      Log.e("Nope", "Cannot draw onto the canvas as it's null");
    }
    else
    {
      Log.e("Draw", "Rendering Should be done successfully?");
      Surface surface = surfaceContainer.getSurface();
      MapFragment.nativeCreateEngine(surface, surface.lockHardwareCanvas()
                                                     .getDensity(), false, true, BuildConfig.VERSION_CODE);
      MapFragment.nativeAttachSurface(surface);
      surfaceContainer.getSurface().unlockCanvasAndPost(canvas);
    }
  }

  @Override
  public void onVisibleAreaChanged(@NonNull Rect visibleArea)
  {
    SurfaceCallback.super.onVisibleAreaChanged(visibleArea);
    Log.e("Something", "Lets see");
  }

  @Override
  public void onStableAreaChanged(@NonNull Rect stableArea)
  {
    SurfaceCallback.super.onStableAreaChanged(stableArea);
  }

  @Override
  public void onSurfaceDestroyed(@NonNull SurfaceContainer surfaceContainer)
  {
    SurfaceCallback.super.onSurfaceDestroyed(surfaceContainer);
  }

  @Override
  public void onScroll(float distanceX, float distanceY)
  {
    SurfaceCallback.super.onScroll(distanceX, distanceY);
  }

  @Override
  public void onFling(float velocityX, float velocityY)
  {
    SurfaceCallback.super.onFling(velocityX, velocityY);
  }

  @Override
  public void onScale(float focusX, float focusY, float scaleFactor)
  {
    SurfaceCallback.super.onScale(focusX, focusY, scaleFactor);
  }

  @Override
  public void onClick(float x, float y)
  {
    SurfaceCallback.super.onClick(x, y);
  }
}