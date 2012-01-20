package com.mapswithme.maps;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.util.Log;
import android.view.MotionEvent;
import android.view.SurfaceHolder;

public class SmartGLSurfaceView extends GLSurfaceView
{
  private static String TAG = "SmartGLSurfaceView";

  private SmartRenderer m_renderer;

  // Commands which are executed on the renderer thread
  private Runnable m_tryToLoadResourcesIfReady;
  private Runnable m_unloadResources;

  public SmartGLSurfaceView(Context context)
  {
    super(context);
    init();
  }

  public SmartGLSurfaceView(Context context, android.util.AttributeSet attrs)
  {
    super(context, attrs);
    init();
  }

  private void init()
  {
    setEGLConfigChooser(true);
    m_renderer = new SmartRenderer(this);
    setRenderer(m_renderer);
    setRenderMode(RENDERMODE_WHEN_DIRTY);

    m_tryToLoadResourcesIfReady = new Runnable()
    {
      @Override
      public void run() { m_renderer.loadResources(); }
    };

    m_unloadResources = new Runnable()
    {
      @Override
      public void run() { m_renderer.unloadResources(); }
    };
  }

  @Override
  public void surfaceCreated (SurfaceHolder holder)
  {
    Log.d(TAG, "surfaceCreated");
    m_renderer.m_isBaseSurfaceReady = false;
    super.surfaceCreated(holder);
  }

  @Override
  public void surfaceChanged (SurfaceHolder holder, int format, int w, int h)
  {
    Log.d(TAG, "surfaceChanged " + w + " " + h);
    m_renderer.m_isBaseSurfaceReady = true;
    super.surfaceChanged(holder, format, w, h);
    queueEvent(m_tryToLoadResourcesIfReady);
    nativeBind(true);
  }

  @Override
  public void surfaceDestroyed (SurfaceHolder holder)
  {
    Log.d(TAG, "surfaceDestroyed");
    nativeBind(false);
    m_renderer.m_isBaseSurfaceReady = false;
    m_renderer.m_isLocalSurfaceReady = false;
    super.surfaceDestroyed(holder);
  }

  @Override
  public void onResume()
  {
    m_renderer.m_isActive = true;
    super.onResume();
    queueEvent(m_tryToLoadResourcesIfReady);
    requestRender();
  }

  @Override
  public void onPause()
  {
    m_renderer.m_isActive = false;
    queueEvent(m_unloadResources);
    super.onPause();
  }

  // Should be called from parent activity
  public void onStop()
  {
    // To avoid releasing OpenGL resources - we've already lost context here
    nativeBind(false);
  }

  @Override
  public void onWindowFocusChanged(boolean hasFocus)
  {
    m_renderer.m_isFocused = hasFocus;
    if (hasFocus)
    {
      queueEvent(m_tryToLoadResourcesIfReady);
      requestRender();
    }
  }

  private final static int START_CMD = 0;
  private final static int DO_CMD = 1;
  private final static int STOP_CMD = 2;

  @Override
  public boolean onTouchEvent (MotionEvent event)
  {
    switch (event.getAction() & MotionEvent.ACTION_MASK)
    {
    case MotionEvent.ACTION_DOWN:
      nativeMove(START_CMD, event.getX(), event.getY());
      break;

    case MotionEvent.ACTION_POINTER_DOWN:
    {
      final float x1, y1, x2, y2;
      if (event.getPointerId(0) < event.getPointerId(1))
      {
        x1 = event.getX(0); y1 = event.getY(0); x2 = event.getX(1); y2 = event.getY(1);
      }
      else
      {
        x1 = event.getX(1); y1 = event.getY(1); x2 = event.getX(0); y2 = event.getY(0);
      }
      nativeZoom(START_CMD, x1, y1, x2, y2);
    }
    break;

    case MotionEvent.ACTION_MOVE:
      if (event.getPointerCount() > 1)
      {
        final float x1, y1, x2, y2;
        if (event.getPointerId(0) < event.getPointerId(1))
        {
          x1 = event.getX(0); y1 = event.getY(0); x2 = event.getX(1); y2 = event.getY(1);
        }
        else
        {
          x1 = event.getX(1); y1 = event.getY(1); x2 = event.getX(0); y2 = event.getY(0);
        }
        nativeZoom(DO_CMD, x1, y1, x2, y2);
      }
      else
      {
        nativeMove(DO_CMD, event.getX(), event.getY());
      }
      break;

    case MotionEvent.ACTION_POINTER_UP:
    {
      final float x1, y1, x2, y2;
      if (event.getPointerId(0) < event.getPointerId(1))
      {
        x1 = event.getX(0); y1 = event.getY(0); x2 = event.getX(1); y2 = event.getY(1);
      }
      else
      {
        x1 = event.getX(1); y1 = event.getY(1); x2 = event.getX(0); y2 = event.getY(0);
      }
      nativeZoom(STOP_CMD, x1, y1, x2, y2);
      final int leftIndex = ((event.getAction() & MotionEvent.ACTION_POINTER_INDEX_MASK)
          >> MotionEvent.ACTION_POINTER_ID_SHIFT) == 0 ? 1 : 0;
      nativeMove(START_CMD, event.getX(leftIndex), event.getY(leftIndex));
    }
    break;

    case MotionEvent.ACTION_UP:
      nativeMove(STOP_CMD, event.getX(), event.getY());
      break;

    case MotionEvent.ACTION_CANCEL:
      if (event.getPointerCount() > 1)
      {
        final float x1, y1, x2, y2;
        if (event.getPointerId(0) < event.getPointerId(1))
        {
          x1 = event.getX(0); y1 = event.getY(0); x2 = event.getX(1); y2 = event.getY(1);
        }
        else
        {
          x1 = event.getX(1); y1 = event.getY(1); x2 = event.getX(0); y2 = event.getY(0);
        }
        nativeZoom(STOP_CMD, x1, y1, x2, y2);
      }
      else
      {
        nativeMove(STOP_CMD, event.getX(), event.getY());
      }
      break;
    }

    requestRender();
    return true;
  }

  // Mode 0 - Start, 1 - Do, 2 - Stop
  private native void nativeMove(int mode, float x, float y);
  private native void nativeZoom(int mode, float x1, float y1, float x2, float y2);
  private native void nativeBind(boolean isBound);
}

class SmartRenderer implements GLSurfaceView.Renderer
{
  private static String TAG = "SmartRenderer";

  // These flags are modified from GUI thread
  public boolean m_isFocused = false;
  public boolean m_isActive = false;
  public boolean m_isBaseSurfaceReady = false;
  // This flag is modified from Renderer thread
  public boolean m_isLocalSurfaceReady = false;

  private boolean m_areResourcesLoaded = false;

  private SmartGLSurfaceView m_view;

  public SmartRenderer(SmartGLSurfaceView view)
  {
    m_view = view;
  }

  public void loadResources()
  {
    // We can load resources even if we don't have the focus
    if (!m_areResourcesLoaded && m_isActive && m_isBaseSurfaceReady && m_isLocalSurfaceReady)
    {
      Log.d(TAG, "***loadResources***");
      nativeLoadResources();
      m_areResourcesLoaded = true;
    }
  }

  public void unloadResources()
  {
    if (m_areResourcesLoaded)
    {
      Log.d(TAG, "***unloadResources***");
      nativeUnloadResources();
      m_areResourcesLoaded = false;
    }
  }

  @Override
  public void onDrawFrame(GL10 gl)
  {
    Log.d(TAG, "onDrawFrame");
    if (m_isActive && m_isLocalSurfaceReady && m_isBaseSurfaceReady)
    {
      if (!m_areResourcesLoaded)
        loadResources();
      if (m_isFocused)
      {
        Log.d(TAG, "***Draw***");
        nativeDrawFrame();
      }
    }
  }

  @Override
  public void onSurfaceChanged(GL10 gl, int width, int height)
  {
    Log.d(TAG, "onSurfaceChanged " + width + " " + height);
    m_isLocalSurfaceReady = true;
    loadResources();
    nativeResize(width, height);
  }

  @Override
  public void onSurfaceCreated(GL10 gl, EGLConfig config)
  {
    Log.d(TAG, "onSurfaceCreated");
    m_isLocalSurfaceReady = false;
  }

  private native void nativeLoadResources();
  private native void nativeUnloadResources();
  private native void nativeDrawFrame();
  private native void nativeResize(int width, int height);
}
