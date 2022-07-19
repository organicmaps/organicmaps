package com.androidAuto;

import android.graphics.Canvas;
import android.graphics.Rect;
import android.util.Log;
import android.view.Surface;
import android.opengl.GLUtils;
import com.mapswithme.util.log.Logger;
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
import com.mapswithme.maps.location.LocationHelper;
import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.egl.EGLSurface;

import java.io.IOException;

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
  {
    Log.e("Show Up", "Shows Up");
    NavigationTemplate.Builder builder = new NavigationTemplate.Builder();
    ActionStrip.Builder actionStripBuilder = new ActionStrip.Builder();
    actionStripBuilder.addAction(new Action.Builder().setTitle("Exit")
                                                     .setOnClickListener(this::exit)
                                                     .build());
    builder.setActionStrip(actionStripBuilder.build());
    NavigationManager navigationManager = getCarContext().getCarService(NavigationManager.class);
    return builder.build();
  }

  private void exit()
  {
    getCarContext().finishCarApp();
  }

  @Override
  public void onSurfaceAvailable(@NonNull SurfaceContainer surfaceContainer)
  {
    MwmApplication cat = MwmApplication.from(getCarContext());
    try
    {
      cat.init();
    }
    catch (IOException e)
    {
      e.printStackTrace();
    }
    int h = surfaceContainer.getHeight();
    int w = surfaceContainer.getWidth();

    if (surfaceContainer.getSurface() != null)
    {
      final boolean firstStart = LocationHelper.INSTANCE.isInFirstRun();
      Log.e("Create Engine", "!");

      //Getting the surface to
      Surface surface = surfaceContainer.getSurface();
      boolean cr = MapFragment.nativeCreateEngine(surface, surface.lockHardwareCanvas()
                                                                  .getDensity(), firstStart, false, BuildConfig.VERSION_CODE);

//
//      EGL10 egl = (EGL10) EGLContext.getEGL();
//      EGLDisplay eglDisplay = egl.eglGetDisplay(EGL10.EGL_DEFAULT_DISPLAY);
//      if (eglDisplay == EGL10.EGL_NO_DISPLAY) {
//        throw new RuntimeException("eglGetDisplay failed: " + GLUtils.getEGLErrorString(egl.eglGetError()));
//      }
//      int[] version = new int[2];
//      if(!egl.eglInitialize(eglDisplay, version)) {
//        throw new RuntimeException("eglInitialize failed: " + GLUtils.getEGLErrorString(egl.eglGetError()));
//      }
//      final int EGL_CONTEXT_CLIENT_VERSION = 0x3098;
//      final int EGL_OPENGL_ES2_BIT = 0x0004;
//      final int MAX_CONFIG_COUNT = 40;
//      int[] configsCount = new int[1];
//      EGLConfig[] configs = new EGLConfig[MAX_CONFIG_COUNT];
//      int[] configSpec = new int[] {
//          EGL10.EGL_RED_SIZE, 8,
//          EGL10.EGL_GREEN_SIZE, 8,
//          EGL10.EGL_BLUE_SIZE, 8,
//          EGL10.EGL_ALPHA_SIZE, 0,
//          EGL10.EGL_STENCIL_SIZE, 0,
//          EGL10.EGL_DEPTH_SIZE, 16,
//          EGL10.EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
//          EGL10.EGL_SURFACE_TYPE, EGL10.EGL_PBUFFER_BIT | EGL10.EGL_WINDOW_BIT,
//          EGL10.EGL_NONE
//      };
//      if (!egl.eglChooseConfig(eglDisplay, configSpec, configs, MAX_CONFIG_COUNT, configsCount)) {
//        throw new IllegalArgumentException("eglChooseConfig failed " +
//                                           GLUtils.getEGLErrorString(egl.eglGetError()));
//      } else if (configsCount[0] == 0) {
//        throw new RuntimeException("eglConfig not initialized");
//      }
//      Log.i(TAG, "Backbuffer format: RGB8");
//      EGLConfig eglConfig = configs[0];
//      int[] contextAttributes = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL10.EGL_NONE };
//      EGLContext eglContext = egl.eglCreateContext(eglDisplay,
//                                                   eglConfig,
//                                                   EGL10.EGL_NO_CONTEXT,
//                                                   contextAttributes);
//      int[] surfaceAttributes = { EGL10.EGL_NONE };
//      EGLSurface eglSurface = egl.eglCreateWindowSurface(eglDisplay,
//                                                         eglConfig,
//                                                         surface,
//                                                         surfaceAttributes);
//      if (eglSurface == null || eglSurface == EGL10.EGL_NO_SURFACE) {
//        int error = egl.eglGetError();
//        String errorString = GLUtils.getEGLErrorString(error);
//        throw new RuntimeException("createWindowSurface failed "
//                                   + GLUtils.getEGLErrorString(error));
//      }
//      Log.i(TAG, "egl success");

      Log.e("Native Create Engine", String.valueOf(cr));
      if (!cat.arePlatformAndCoreInitialized())
      {
        Log.e("arePlatformAndCoreInitialized()", String.valueOf(cat.arePlatformAndCoreInitialized()));
        Log.e("Nope", "Cannot draw onto the canvas as it's null");
      }
      else
      {
        Log.e("arePlatformAndCoreInitialized()", String.valueOf(cat.arePlatformAndCoreInitialized()));
        Log.e("Draw", "Rendering Should be done successfully?");

        try
        {
          Log.e("Attcing Surafce", "!!!");
          MapFragment.nativeAttachSurface(surface);
        }
        catch (Exception e)
        {
          e.printStackTrace();
        }
      }
    }
    else
    {
      Log.e("NPE", "Surface Not Available");
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