package com.nvidia.devtech;

import com.mapswithme.maps.R;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.MotionEvent;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGL11;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.egl.EGLSurface;

import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.SurfaceHolder.Callback;

public abstract class NvEventQueueActivity extends Activity
{
  private static final String TAG = "NvEventQueueActivity";
  private static final int EGL_CONTEXT_CLIENT_VERSION = 0x3098;

  private EGL10 m_egl = (EGL10) EGLContext.getEGL();
  protected boolean m_eglInitialized = false;
  protected EGLSurface m_eglSurface = null;
  protected EGLDisplay m_eglDisplay = null;
  protected EGLContext m_eglContext = null;
  protected EGLConfig m_eglConfig = null;

  protected SurfaceHolder m_cachedSurfaceHolder = null;
  private int m_surfaceWidth = 0;
  private int m_surfaceHeight = 0;

  private int m_fixedWidth = 0;
  private int m_fixedHeight = 0;

  private boolean m_nativeLaunched = false;

  public void setFixedSize(int fw, int fh)
  {
    m_fixedWidth = fw;
    m_fixedHeight = fh;
  }

  public int getSurfaceWidth()
  {
    return m_surfaceWidth;
  }

  public int getSurfaceHeight()
  {
    return m_surfaceHeight;
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
        m_cachedSurfaceHolder = holder;

        if (m_fixedWidth != 0 && m_fixedHeight != 0)
        {
          System.out.println("Setting fixed window size");
          holder.setFixedSize(m_fixedWidth, m_fixedHeight);
        }

        onSurfaceCreatedNative(m_surfaceWidth, m_surfaceHeight);
      }

      // @Override
      public void surfaceChanged(SurfaceHolder holder, int format, int width,
          int height)
      {
        m_cachedSurfaceHolder = holder;
        System.out.println("**** Surface changed: " + width + ", " + height);
        m_surfaceWidth = width;
        m_surfaceHeight = height;
        onSurfaceChangedNative(m_surfaceWidth, m_surfaceHeight);
      }

      // @Override
      public void surfaceDestroyed(SurfaceHolder holder)
      {
        m_cachedSurfaceHolder = null;
        System.out.println("**** systemInit.surfaceDestroyed");
        onSurfaceDestroyedNative();
      }
    });

    m_nativeLaunched = true;
    onCreateNative();
  }

  @Override
  protected void onStart()
  {
    System.out.println("**** onStart");
    super.onStart();

    if (m_nativeLaunched)
      onStartNative();
  }

  @Override
  protected void onRestart()
  {
    System.out.println("**** onRestart");
    super.onRestart();

    if (m_nativeLaunched)
      onRestartNative();
  }

  @Override
  protected void onResume()
  {
    System.out.println("**** onResume");
    super.onResume();
    if (m_nativeLaunched)
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
    if (m_nativeLaunched)
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
    if (m_nativeLaunched)
      onPauseNative();
  }

  @Override
  protected void onStop()
  {
    System.out.println("**** onStop");
    super.onStop();

    if (m_nativeLaunched)
      onStopNative();
  }

  @Override
  public void onDestroy()
  {
    System.out.println("**** onDestroy");
    super.onDestroy();

    if (m_nativeLaunched)
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
    if (!m_nativeLaunched || count == 0)
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
  protected int stencilSize = 0;
  /** The number of bits requested for the depth component */
  protected int depthSize = 16;

  /**
   * Called to initialize EGL. This function should not be called by the
   * inheriting activity, but can be overridden if needed.
   * 
   * @return True if successful
   */
  protected boolean InitEGL()
  {
    final int[] configAttrs = new int[] { EGL11.EGL_RED_SIZE, redSize,
                                          EGL11.EGL_GREEN_SIZE, greenSize,
                                          EGL11.EGL_BLUE_SIZE, blueSize,
                                          EGL11.EGL_ALPHA_SIZE, alphaSize,
                                          EGL11.EGL_STENCIL_SIZE, stencilSize,
                                          EGL11.EGL_DEPTH_SIZE, depthSize,
                                          EGL11.EGL_CONFIG_CAVEAT, EGL11.EGL_NONE,
                                          EGL11.EGL_NONE };

    m_eglDisplay = m_egl.eglGetDisplay(EGL11.EGL_DEFAULT_DISPLAY);
    if (m_eglDisplay == EGL11.EGL_NO_DISPLAY)
    {
      Log.d(TAG, "eglGetDisplay failed");
      return false;
    }
    
    int[] version = new int[2];
    if (!m_egl.eglInitialize(m_eglDisplay, version))
    {
      Log.d(TAG, "eglInitialize failed with error " + m_egl.eglGetError());
      return false;
    }

    final EGLConfig[] configs = new EGLConfig[40];
    final int actualConfigsNumber[] = new int[] {0};
    if (!m_egl.eglChooseConfig(m_eglDisplay, configAttrs, configs, configs.length, actualConfigsNumber))
    {
      Log.d(TAG, "eglChooseConfig failed with error " + m_egl.eglGetError());
      return false;
    }

    if (actualConfigsNumber[0] == 0)
    {
      Log.d(TAG, "eglChooseConfig returned zero configs");
      return false;
    }

    
    m_eglConfig = configs[0];
    // Simple check - choose first config with matched depth buffer
    int[] value = new int[1];
    for (int i = 0; i < actualConfigsNumber[0]; ++i)
    {
      m_egl.eglGetConfigAttrib(m_eglDisplay, configs[i], EGL11.EGL_DEPTH_SIZE, value);
      if (value[0] == depthSize)
      {
        m_eglConfig = configs[i];
        break;
      }
    }
    
    final int[] contextAttrs = new int[] { EGL_CONTEXT_CLIENT_VERSION, 1, EGL11.EGL_NONE };
    m_eglContext = m_egl.eglCreateContext(m_eglDisplay, m_eglConfig, EGL11.EGL_NO_CONTEXT, contextAttrs);
    if (m_eglContext == EGL11.EGL_NO_CONTEXT)
    {
      Log.d(TAG, "eglCreateContext failed with error " + m_egl.eglGetError());
      return false;
    }

    m_eglInitialized = true;

    return true;
  }

  /**
   * Called to clean up m_egl. This function should not be called by the
   * inheriting activity, but can be overridden if needed.
   */
  protected boolean CleanupEGL()
  {
    Log.d(TAG, "CleanupEGL");

    if (!m_eglInitialized)
      return false;

    if (!DestroySurfaceEGL())
      return false;

    if (m_eglDisplay != null)
      m_egl.eglMakeCurrent(m_eglDisplay, EGL11.EGL_NO_SURFACE,
          EGL11.EGL_NO_SURFACE, EGL11.EGL_NO_CONTEXT);
    if (m_eglContext != null)
    {
      Log.d(TAG, "eglDestroyContext");
      m_egl.eglDestroyContext(m_eglDisplay, m_eglContext);
    }
    if (m_eglDisplay != null)
      m_egl.eglTerminate(m_eglDisplay);

    m_eglDisplay = null;
    m_eglContext = null;
    m_eglSurface = null;

    m_eglConfig = null;

    m_surfaceWidth = 0;
    m_surfaceHeight = 0;

    m_eglInitialized = false;

    return true;
  }

  protected boolean CreateSurfaceEGL()
  {
    if (m_cachedSurfaceHolder == null)
    {
      Log.d(TAG, "createEGLSurface failed, m_cachedSurfaceHolder is null");
      return false;
    }

    if (!m_eglInitialized && (m_eglInitialized = InitEGL()))
    {
      Log.d(TAG, "createEGLSurface failed, cannot initialize EGL");
      return false;
    }

    if (m_eglDisplay == null)
    {
      Log.d(TAG, "createEGLSurface: display is null");
      return false;
    }
    else if (m_eglConfig == null)
    {
      Log.d(TAG, "createEGLSurface: config is null");
      return false;
    }
    
    m_eglSurface = m_egl.eglCreateWindowSurface(m_eglDisplay, m_eglConfig, m_cachedSurfaceHolder, null);
    if (m_eglSurface == EGL11.EGL_NO_SURFACE)
    {
      Log.d(TAG, "eglCreateWindowSurface failed with error " + m_egl.eglGetError());
      return false;
    }
    
    int sizes[] = new int[1];
    m_egl.eglQuerySurface(m_eglDisplay, m_eglSurface, EGL10.EGL_WIDTH, sizes);
    m_surfaceWidth = sizes[0];
    m_egl.eglQuerySurface(m_eglDisplay, m_eglSurface, EGL10.EGL_HEIGHT, sizes);
    m_surfaceHeight = sizes[0];

    return true;
  }

  /**
   * Destroys the EGLSurface used for rendering. This function should not be
   * called by the inheriting activity, but can be overridden if needed.
   */
  protected boolean DestroySurfaceEGL()
  {
    if (m_eglDisplay != null && m_eglSurface != null)
      m_egl.eglMakeCurrent(m_eglDisplay, EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_SURFACE, m_eglContext);
    if (m_eglSurface != null)
      m_egl.eglDestroySurface(m_eglDisplay, m_eglSurface);
    m_eglSurface = null;

    return true;
  }

  public boolean BindSurfaceAndContextEGL()
  {
    if (m_eglContext == null)
    {
      Log.d(TAG, "m_eglContext is NULL");
      return false;
    }
    else if (m_eglSurface == null)
    {
      Log.d(TAG, "m_eglSurface is NULL");
      return false;
    }
    else if (!m_egl.eglMakeCurrent(m_eglDisplay, m_eglSurface, m_eglSurface, m_eglContext))
    {
      Log.d(TAG, "eglMakeCurrent err: " + m_egl.eglGetError());
      return false;
    }

    return true;
  }

  public boolean UnbindSurfaceAndContextEGL()
  {
    Log.d(TAG, "UnbindSurfaceAndContextEGL");
    if (m_eglDisplay == null)
    {
      System.out.println("UnbindSurfaceAndContextEGL: display is null");
      return false;
    }

    if (!m_egl.eglMakeCurrent(m_eglDisplay, EGL10.EGL_NO_SURFACE,
        EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_CONTEXT))
    {
      System.out.println("m_egl(Un)MakeCurrent err: " + m_egl.eglGetError());
      return false;
    }

    return true;
  }

  public boolean SwapBuffersEGL()
  {
    // long stopTime;
    // long startTime = nvGetSystemTime();
    if (m_eglSurface == null)
    {
      Log.d(TAG, "m_eglSurface is NULL");
      return false;
    }
    else if (!m_egl.eglSwapBuffers(m_eglDisplay, m_eglSurface))
    {
      Log.d(TAG, "eglSwapBufferrr: " + m_egl.eglGetError());
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
    return m_egl.eglGetError();
  }
}
