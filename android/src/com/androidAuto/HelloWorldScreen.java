package com.androidAuto;

import android.content.Context;
import android.graphics.Rect;
import android.util.Log;
import android.view.Surface;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.car.app.AppManager;
import androidx.car.app.CarContext;
import androidx.car.app.Screen;
import androidx.car.app.SurfaceCallback;
import androidx.car.app.SurfaceContainer;
import androidx.car.app.model.Action;
import androidx.car.app.model.ActionStrip;
import androidx.car.app.model.CarIcon;
import androidx.car.app.model.Template;
import androidx.car.app.navigation.NavigationManager;
import androidx.car.app.navigation.model.NavigationTemplate;
import androidx.core.graphics.drawable.IconCompat;
import com.mapswithme.maps.BuildConfig;
import com.mapswithme.maps.MapFragment;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;

import java.io.IOException;

public class HelloWorldScreen extends Screen implements SurfaceCallback
{
  private static final String TAG = HelloWorldScreen.class.getSimpleName();
  @Nullable
  private MapFragment mMapFragment;
  private SurfaceCallback mSurfaceCallback;

  public HelloWorldScreen(@NonNull CarContext carContext)
  {
    super(carContext);

    MwmApplication cat = MwmApplication.from(carContext);
    try
    {
      cat.init();
    }
    catch (IOException e)
    {
      e.printStackTrace();
      return;
    }

    carContext.getCarService(AppManager.class).setSurfaceCallback(this);
  }

  @NonNull
  @Override
  public Template onGetTemplate()
  {
    Log.e("Show Up", "Shows Up");
    NavigationTemplate.Builder builder = new NavigationTemplate.Builder();
    ActionStrip.Builder actionStripBuilder = new ActionStrip.Builder();
    actionStripBuilder.addAction(new Action.Builder().setTitle("Exit")
                                                     .setOnClickListener(this::exit)
                                                     .build());

    Action panAction = new Action.Builder(Action.PAN).build();
    Action zoomIn = new Action.Builder().setIcon(new CarIcon.Builder(IconCompat.createWithResource(getCarContext(), R.drawable.ic_zoom_in)).build())
                                        .setOnClickListener(this::zoomIn)
                                        .build();
    Action zoomOut = new Action.Builder().setIcon(new CarIcon.Builder(IconCompat.createWithResource(getCarContext(), R.drawable.ic_zoom_out)).build())
                                         .setOnClickListener(this::zoomOut)
                                         .build();
    ActionStrip mapActionStrip = new ActionStrip.Builder().addAction(zoomIn)
                                                          .addAction(zoomOut)
                                                          .addAction(panAction)
                                                          .build();
    builder.setMapActionStrip(mapActionStrip);
    builder.setActionStrip(actionStripBuilder.build());
    NavigationManager navigationManager = getCarContext().getCarService(NavigationManager.class);
    return builder.build();
  }

  private void zoomOut()
  {
    MapFragment.nativeScaleMinus();
  }

  private void zoomIn()
  {
    MapFragment.nativeScalePlus();
  }

  private void exit()
  {
    getCarContext().finishCarApp();
    MapFragment.nativeDetachSurface(true);
  }

  @Override
  public void onSurfaceAvailable(@NonNull SurfaceContainer surfaceContainer)
  {
    Log.i(TAG, "Surface available " + surfaceContainer);

    final Surface surface = surfaceContainer.getSurface();
    if (surface == null)
    {
      Log.e(TAG, "Surface is NULL");
      return;
    }

    if (MapFragment.nativeIsEngineCreated())
    {
      if (!MapFragment.nativeAttachSurface(surface))
      {
        reportUnsupported();
        return;
      }
      MapFragment.nativeResumeSurfaceRendering();
      return;
    }

    final int WIDGET_COPYRIGHT = 0x04;
    MapFragment.nativeSetupWidget(WIDGET_COPYRIGHT, 0.0f, 0.0f, 0);

    if (!MapFragment.nativeCreateEngine(surface, surfaceContainer.getDpi(), true, false, BuildConfig.VERSION_CODE))
    {
      reportUnsupported();
      return;
    }

    MapFragment.nativeResumeSurfaceRendering();
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
    MapFragment.nativeDetachSurface(true);
    Log.i(TAG, "Surface destroyed");
  }

  @Override
  public void onScroll(float distanceX, float distanceY)
  {
    SurfaceCallback.super.onScroll(distanceX, distanceY);
    Log.i("Scroll", "Scrolled ");
  }

  @Override
  public void onFling(float velocityX, float velocityY)
  {
    SurfaceCallback.super.onFling(velocityX, velocityY);
    Log.i("Fling", "Flinged");
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

  private void reportUnsupported()
  {
    final Context context = getCarContext();
    Log.e(TAG, context.getString(R.string.unsupported_phone));
  }
}