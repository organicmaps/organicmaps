//----------------------------------------------------------------------------------
// File:            libs\src\com\nvidia\devtech\NvEventQueueActivity.java
// Samples Version: NVIDIA Android Lifecycle samples 1_0beta 
// Email:           tegradev@nvidia.com
// Web:             http://developer.nvidia.com/category/zone/mobile-development
//
// Copyright 2009-2011 NVIDIA� Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
//----------------------------------------------------------------------------------
package com.nvidia.devtech;

import com.mapswithme.maps.R;

import android.app.Activity;
import android.os.Bundle;
import android.view.MotionEvent;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.egl.EGLSurface;
import javax.microedition.khronos.opengles.GL11;

import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.SurfaceHolder.Callback;

public abstract class NvEventQueueActivity extends Activity
{
  // private static final int EGL_RENDERABLE_TYPE = 0x3040;
  // private static final int EGL_OPENGL_ES_BIT = 0x0001;
  // private static final int EGL_OPENGL_ES2_BIT = 0x0004;
  private static final int EGL_CONTEXT_CLIENT_VERSION = 0x3098;
  EGL10 egl = null;
  GL11 gl = null;

  protected boolean eglInitialized = false;
  protected EGLSurface eglSurface = null;
  protected EGLDisplay eglDisplay = null;
  protected EGLContext eglContext = null;
  protected EGLConfig eglConfig = null;

  protected SurfaceHolder cachedSurfaceHolder = null;
  private int surfaceWidth = 0;
  private int surfaceHeight = 0;

  private int fixedWidth = 0;
  private int fixedHeight = 0;

  private boolean nativeLaunched = false;

  public void setFixedSize(int fw, int fh)
  {
    fixedWidth = fw;
    fixedHeight = fh;
  }

  public int getSurfaceWidth()
  {
    return surfaceWidth;
  }

  public int getSurfaceHeight()
  {
    return surfaceHeight;
  }

  protected native boolean onCreateNative();

  protected native boolean onStartNative();

  protected native boolean onRestartNative();

  protected native boolean onResumeNative();

  protected native boolean onSurfaceCreatedNative(int w, int h);

  protected native boolean onFocusChangedNative(boolean focused);

  protected native boolean onSurfaceChangedNative(int w, int h);

  protected native boolean onSurfaceDestroyedNative();

  protected native boolean onPauseNative();

  protected native boolean onStopNative();

  protected native boolean onDestroyNative();

  public native boolean multiTouchEvent(int action, boolean hasFirst,
      boolean hasSecond, int x0, int y0, int x1, int y1, MotionEvent event);

  @Override
  public void onCreate(Bundle savedInstanceState)
  {
    System.out.println("**** onCreate");
    super.onCreate(savedInstanceState);

    setContentView(R.layout.map);
    final SurfaceView surfaceView = (SurfaceView) findViewById(R.id.map_surfaceview);

    SurfaceHolder holder = surfaceView.getHolder();
    holder.setType(SurfaceHolder.SURFACE_TYPE_GPU);

    holder.addCallback(new Callback()
    {
      // @Override
      public void surfaceCreated(SurfaceHolder holder)
      {
        System.out.println("**** systemInit.surfaceCreated");
        cachedSurfaceHolder = holder;

        if (fixedWidth != 0 && fixedHeight != 0)
        {
          System.out.println("Setting fixed window size");
          holder.setFixedSize(fixedWidth, fixedHeight);
        }

        onSurfaceCreatedNative(surfaceWidth, surfaceHeight);
      }

      // @Override
      public void surfaceChanged(SurfaceHolder holder, int format, int width,
          int height)
      {
        cachedSurfaceHolder = holder;
        System.out.println("**** Surface changed: " + width + ", " + height);
        surfaceWidth = width;
        surfaceHeight = height;
        onSurfaceChangedNative(surfaceWidth, surfaceHeight);
      }

      // @Override
      public void surfaceDestroyed(SurfaceHolder holder)
      {
        cachedSurfaceHolder = null;
        System.out.println("**** systemInit.surfaceDestroyed");
        onSurfaceDestroyedNative();
      }
    });

    nativeLaunched = true;
    onCreateNative();
  }

  @Override
  protected void onStart()
  {
    System.out.println("**** onStart");
    super.onStart();

    if (nativeLaunched)
      onStartNative();
  }

  @Override
  protected void onRestart()
  {
    System.out.println("**** onRestart");
    super.onRestart();

    if (nativeLaunched)
      onRestartNative();
  }

  @Override
  protected void onResume()
  {
    System.out.println("**** onResume");
    super.onResume();
    if (nativeLaunched)
      onResumeNative();
  }

  @Override
  public void onLowMemory()
  {
    System.out.println("**** onLowMemory");
    super.onLowMemory();
  }

  @Override
  public void onWindowFocusChanged(boolean hasFocus)
  {
    System.out.println("**** onWindowFocusChanged ("
        + ((hasFocus == true) ? "TRUE" : "FALSE") + ")");
    if (nativeLaunched)
      onFocusChangedNative(hasFocus);
    super.onWindowFocusChanged(hasFocus);
  }

  @Override
  protected void onSaveInstanceState(Bundle outState)
  {
    System.out.println("**** onSaveInstanceState");
    super.onSaveInstanceState(outState);
  }

  @Override
  protected void onPause()
  {
    System.out.println("**** onPause");
    super.onPause();
    if (nativeLaunched)
      onPauseNative();
  }

  @Override
  protected void onStop()
  {
    System.out.println("**** onStop");
    super.onStop();

    if (nativeLaunched)
      onStopNative();
  }

  @Override
  public void onDestroy()
  {
    System.out.println("**** onDestroy");
    super.onDestroy();

    if (nativeLaunched)
    {
      onDestroyNative();

      CleanupEGL();
    }
  }

  private int m_lastPointerId = 0;

  @Override
  public boolean onTouchEvent(MotionEvent event)
  {
    final int count = event.getPointerCount();
    if (!nativeLaunched || count == 0)
      return super.onTouchEvent(event);

    switch (count)
    {
    case 1:
      m_lastPointerId = event.getPointerId(0);
      return multiTouchEvent(event.getAction(), true, false,
          (int)event.getX(), (int)event.getY(), 0, 0, event);

    default:
      {
        if (event.getPointerId(0) == m_lastPointerId)
          return multiTouchEvent(event.getAction(), true, true,
              (int)event.getX(0), (int)event.getY(0),
              (int)event.getX(1), (int)event.getY(1), event);
        else
          return multiTouchEvent(event.getAction(), true, true,
              (int)event.getX(1), (int)event.getY(1),
              (int)event.getX(0), (int)event.getY(0), event);

      }
    }
  }
  
  /** The number of bits requested for the red component */
  protected int redSize = 5;
  /** The number of bits requested for the green component */
  protected int greenSize = 6;
  /** The number of bits requested for the blue component */
  protected int blueSize = 5;
  /** The number of bits requested for the alpha component */
  protected int alphaSize = 0;
  /** The number of bits requested for the stencil component */
  protected int stencilSize = 8;
  /** The number of bits requested for the depth component */
  protected int depthSize = 16;

  /** Attributes used when selecting the EGLConfig */
  protected int[] configAttrs = null;
  /** Attributes used when creating the context */
  protected int[] contextAttrs = null;

  /**
   * Called to initialize EGL. This function should not be called by the
   * inheriting activity, but can be overridden if needed.
   * 
   * @return True if successful
   */
  protected boolean InitEGL()
  {
    if (configAttrs == null)
      configAttrs = new int[]
      { EGL10.EGL_NONE };
    int[] oldConf = configAttrs;

    configAttrs = new int[3 + oldConf.length - 1];
    int i = 0;
    for (i = 0; i < oldConf.length - 1; i++)
      configAttrs[i] = oldConf[i];
    configAttrs[i++] = EGL10.EGL_RENDERABLE_TYPE;
    configAttrs[i++] = 1;
    configAttrs[i++] = EGL10.EGL_NONE;

    contextAttrs = new int[]
    { EGL_CONTEXT_CLIENT_VERSION, 1, EGL10.EGL_NONE };

    int[] oldConfES = configAttrs;

    configAttrs = new int[13 + oldConfES.length - 1];

    for (i = 0; i < oldConfES.length - 1; i++)
      configAttrs[i] = oldConfES[i];
    configAttrs[i++] = EGL10.EGL_RED_SIZE;
    configAttrs[i++] = redSize;
    configAttrs[i++] = EGL10.EGL_GREEN_SIZE;
    configAttrs[i++] = greenSize;
    configAttrs[i++] = EGL10.EGL_BLUE_SIZE;
    configAttrs[i++] = blueSize;
    configAttrs[i++] = EGL10.EGL_ALPHA_SIZE;
    configAttrs[i++] = alphaSize;
    configAttrs[i++] = EGL10.EGL_STENCIL_SIZE;
    configAttrs[i++] = stencilSize;
    configAttrs[i++] = EGL10.EGL_DEPTH_SIZE;
    configAttrs[i++] = depthSize;
    configAttrs[i++] = EGL10.EGL_NONE;

    egl = (EGL10) EGLContext.getEGL();
    System.out.println("getEGL:" + egl.eglGetError());

    eglDisplay = egl.eglGetDisplay(EGL10.EGL_DEFAULT_DISPLAY);
    System.out.println("eglDisplay: " + eglDisplay + ", err: "
        + egl.eglGetError());

    int[] version = new int[2];
    boolean ret = egl.eglInitialize(eglDisplay, version);

    System.out.println("eglInitialize returned: " + ret);
    if (!ret)
      return false;

    int eglErr = egl.eglGetError();
    if (eglErr != EGL10.EGL_SUCCESS)
      return false;

    System.out.println("eglInitialize err: " + eglErr);

    final EGLConfig[] config = new EGLConfig[20];
    int num_configs[] = new int[1];
    egl.eglChooseConfig(eglDisplay, configAttrs, config, config.length,
        num_configs);
    System.out.println("eglChooseConfig err: " + egl.eglGetError());

    int score = 1 << 24; // to make sure even worst score is better than this,
                         // like 8888 when request 565...
    int val[] = new int[1];
    for (i = 0; i < num_configs[0]; i++)
    {
      boolean cont = true;
      int currScore = 0;
      int r, g, b, a, d, s;
      for (int j = 0; j < (oldConf.length - 1) >> 1; j++)
      {
        egl.eglGetConfigAttrib(eglDisplay, config[i], configAttrs[j * 2], val);
        if ((val[0] & configAttrs[j * 2 + 1]) != configAttrs[j * 2 + 1])
        {
          cont = false; // Doesn't match the "must have" configs
          break;
        }
      }
      if (!cont)
        continue;
      egl.eglGetConfigAttrib(eglDisplay, config[i], EGL10.EGL_RED_SIZE, val);
      r = val[0];
      egl.eglGetConfigAttrib(eglDisplay, config[i], EGL10.EGL_GREEN_SIZE, val);
      g = val[0];
      egl.eglGetConfigAttrib(eglDisplay, config[i], EGL10.EGL_BLUE_SIZE, val);
      b = val[0];
      egl.eglGetConfigAttrib(eglDisplay, config[i], EGL10.EGL_ALPHA_SIZE, val);
      a = val[0];
      egl.eglGetConfigAttrib(eglDisplay, config[i], EGL10.EGL_DEPTH_SIZE, val);
      d = val[0];
      egl.eglGetConfigAttrib(eglDisplay, config[i], EGL10.EGL_STENCIL_SIZE, val);
      s = val[0];

      System.out.println(">>> EGL Config [" + i + "] R" + r + "G" + g + "B" + b
          + "A" + a + " D" + d + "S" + s);

      currScore = (Math.abs(r - redSize) + Math.abs(g - greenSize)
          + Math.abs(b - blueSize) + Math.abs(a - alphaSize)) << 16;
      currScore += Math.abs(d - depthSize) << 8;
      currScore += Math.abs(s - stencilSize);

      if (currScore < score)
      {
        System.out.println("--------------------------");
        System.out.println("New config chosen: " + i);
        for (int j = 0; j < (configAttrs.length - 1) >> 1; j++)
        {
          egl.eglGetConfigAttrib(eglDisplay, config[i], configAttrs[j * 2], val);
          if (val[0] >= configAttrs[j * 2 + 1])
            System.out.println("setting " + j + ", matches: " + val[0]);
        }

        score = currScore;
        eglConfig = config[i];
      }
    }

    eglContext = egl.eglCreateContext(eglDisplay, eglConfig,
        EGL10.EGL_NO_CONTEXT, contextAttrs);
    System.out.println("eglCreateContext: " + egl.eglGetError());

    gl = (GL11) eglContext.getGL();

    eglInitialized = true;

    return true;
  }

  /**
   * Called to clean up egl. This function should not be called by the
   * inheriting activity, but can be overridden if needed.
   */
  protected boolean CleanupEGL()
  {
    System.out.println("cleanupEGL");

    if (!eglInitialized)
      return false;

    if (!DestroySurfaceEGL())
      return false;

    if (eglDisplay != null)
      egl.eglMakeCurrent(eglDisplay, EGL10.EGL_NO_SURFACE,
          EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_CONTEXT);
    if (eglContext != null)
    {
      System.out.println("Destroy Context");
      egl.eglDestroyContext(eglDisplay, eglContext);
    }
    if (eglDisplay != null)
      egl.eglTerminate(eglDisplay);

    eglDisplay = null;
    eglContext = null;
    eglSurface = null;

    eglConfig = null;

    surfaceWidth = 0;
    surfaceHeight = 0;

    eglInitialized = false;

    return true;
  }

  protected boolean CreateSurfaceEGL()
  {
    if (cachedSurfaceHolder == null)
    {
      System.out
          .println("createEGLSurface failed, cachedSurfaceHolder is null");
      return false;
    }

    if (!eglInitialized && (eglInitialized = InitEGL()))
    {
      System.out.println("createEGLSurface failed, cannot initialize EGL");
      return false;
    }

    if (eglDisplay == null)
    {
      System.out.println("createEGLSurface: display is null");
      return false;
    } else if (eglConfig == null)
    {
      System.out.println("createEGLSurface: config is null");
      return false;
    }
    eglSurface = egl.eglCreateWindowSurface(eglDisplay, eglConfig,
        cachedSurfaceHolder, null);
    System.out.println("eglSurface: " + eglSurface + ", err: "
        + egl.eglGetError());
    int sizes[] = new int[1];

    egl.eglQuerySurface(eglDisplay, eglSurface, EGL10.EGL_WIDTH, sizes);
    surfaceWidth = sizes[0];
    egl.eglQuerySurface(eglDisplay, eglSurface, EGL10.EGL_HEIGHT, sizes);
    surfaceHeight = sizes[0];

    return true;
  }

  /**
   * Destroys the EGLSurface used for rendering. This function should not be
   * called by the inheriting activity, but can be overridden if needed.
   */
  protected boolean DestroySurfaceEGL()
  {
    if (eglDisplay != null && eglSurface != null)
      egl.eglMakeCurrent(eglDisplay, EGL10.EGL_NO_SURFACE,
          EGL10.EGL_NO_SURFACE, eglContext);
    if (eglSurface != null)
      egl.eglDestroySurface(eglDisplay, eglSurface);
    eglSurface = null;

    return true;
  }

  public boolean BindSurfaceAndContextEGL()
  {
    if (eglContext == null)
    {
      System.out.println("eglContext is NULL");
      return false;
    } else if (eglSurface == null)
    {
      System.out.println("eglSurface is NULL");
      return false;
    } else if (!egl.eglMakeCurrent(eglDisplay, eglSurface, eglSurface,
        eglContext))
    {
      System.out.println("eglMakeCurrent err: " + egl.eglGetError());
      return false;
    }

    return true;
  }

  public boolean UnbindSurfaceAndContextEGL()
  {
    System.out.println("UnbindSurfaceAndContextEGL");
    if (eglDisplay == null)
    {
      System.out.println("UnbindSurfaceAndContextEGL: display is null");
      return false;
    }

    if (!egl.eglMakeCurrent(eglDisplay, EGL10.EGL_NO_SURFACE,
        EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_CONTEXT))
    {
      System.out.println("egl(Un)MakeCurrent err: " + egl.eglGetError());
      return false;
    }

    return true;
  }

  public boolean SwapBuffersEGL()
  {
    // long stopTime;
    // long startTime = nvGetSystemTime();
    if (eglSurface == null)
    {
      System.out.println("eglSurface is NULL");
      return false;
    } else if (!egl.eglSwapBuffers(eglDisplay, eglSurface))
    {
      System.out.println("eglSwapBufferrr: " + egl.eglGetError());
      return false;
    }
    // stopTime = nvGetSystemTime();
    // String s = String.format("%d ms in eglSwapBuffers", (int)(stopTime -
    // startTime));
    // Log.v("EventAccelerometer", s);

    return true;
  }

  public int GetErrorEGL()
  {
    return egl.eglGetError();
  }
}
