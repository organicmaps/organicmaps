package com.nvidia.devtech;

import android.os.Bundle;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceHolder.Callback;
import android.view.SurfaceView;
import android.view.View;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.MapsWithMeBaseActivity;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.StubLogger;

public abstract class NvEventQueueActivity extends MapsWithMeBaseActivity implements View.OnTouchListener
{
  private static final String TAG = "NvEventQueueActivity";

  private final Logger mLog = StubLogger.get();

  private EglWrapper mEglWrapper = null;
  protected SurfaceHolder m_cachedSurfaceHolder = null;
  private int m_surfaceWidth = 0;
  private int m_surfaceHeight = 0;

  private int m_displayDensity = 0;

  private int m_fixedWidth = 0;
  private int m_fixedHeight = 0;

  private boolean m_nativeLaunched = false;

  public void setFixedSize(int fw, int fh)
  {
    m_fixedWidth = fw;
    m_fixedHeight = fh;
  }

  public int getDisplayDensity()
  {
    return m_displayDensity;
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

  protected native boolean onSurfaceCreatedNative(int w, int h, int density);

  protected native boolean onFocusChangedNative(boolean focused);

  protected native boolean onSurfaceChangedNative(int w, int h, int density);

  protected native boolean onSurfaceDestroyedNative();

  protected native boolean onPauseNative();

  protected native boolean onStopNative();

  protected native boolean onDestroyNative();

  public native boolean multiTouchEvent(int action, boolean hasFirst,
                                        boolean hasSecond, int x0, int y0, int x1, int y1, MotionEvent event);

  @SuppressWarnings("deprecation")
  @Override
  public void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);

    final SurfaceView surfaceView = (SurfaceView) findViewById(R.id.map_surfaceview);
    surfaceView.setOnTouchListener(this);

    final SurfaceHolder holder = surfaceView.getHolder();
    holder.setType(SurfaceHolder.SURFACE_TYPE_GPU);

    final DisplayMetrics metrics = new DisplayMetrics();
    getWindowManager().getDefaultDisplay().getMetrics(metrics);
    m_displayDensity = metrics.densityDpi;

    holder.addCallback(new Callback()
    {
      @Override
      public void surfaceCreated(SurfaceHolder holder)
      {
        m_cachedSurfaceHolder = holder;
        if (m_fixedWidth != 0 && m_fixedHeight != 0)
          holder.setFixedSize(m_fixedWidth, m_fixedHeight);
        onSurfaceCreatedNative(m_surfaceWidth, m_surfaceHeight, getDisplayDensity());
      }

      @Override
      public void surfaceChanged(SurfaceHolder holder, int format, int width, int height)
      {
        m_cachedSurfaceHolder = holder;
        m_surfaceWidth = width;
        m_surfaceHeight = height;
        onSurfaceChangedNative(m_surfaceWidth, m_surfaceHeight, getDisplayDensity());
      }

      @Override
      public void surfaceDestroyed(SurfaceHolder holder)
      {
        m_cachedSurfaceHolder = null;
        onSurfaceDestroyedNative();
      }
    });

    m_nativeLaunched = true;
    onCreateNative();
  }

  @Override
  protected void onStart()
  {
    super.onStart();
    if (m_nativeLaunched)
      onStartNative();
  }

  @Override
  protected void onRestart()
  {
    super.onRestart();
    if (m_nativeLaunched)
      onRestartNative();
  }

  @Override
  protected void onResume()
  {
    super.onResume();
    if (m_nativeLaunched)
      onResumeNative();
  }

  @Override
  public void onWindowFocusChanged(boolean hasFocus)
  {
    if (m_nativeLaunched)
      onFocusChangedNative(hasFocus);
    super.onWindowFocusChanged(hasFocus);
  }

  @Override
  protected void onPause()
  {
    super.onPause();
    if (m_nativeLaunched)
      onPauseNative();
  }

  @Override
  protected void onStop()
  {
    super.onStop();
    if (m_nativeLaunched)
      onStopNative();
  }

  @Override
  public void onDestroy()
  {
    super.onDestroy();
    if (m_nativeLaunched)
    {
      onDestroyNative();
      CleanupEGL();
    }
  }

  private int m_lastPointerId = 0;


  @Override
  public boolean onTouch(View v, MotionEvent event)
  {
    final int count = event.getPointerCount();

    if (!m_nativeLaunched || count == 0)
      return super.onTouchEvent(event);

    switch (count)
    {
    case 1:
    {
      m_lastPointerId = event.getPointerId(0);

      final int x0 = (int) event.getX();
      final int y0 = (int) event.getY();

      return multiTouchEvent(event.getAction(), true, false, x0, y0, 0, 0, event);
    }
    default:
    {
      final int x0 = (int) event.getX(0);
      final int y0 = (int) event.getY(0);

      final int x1 = (int) event.getX(1);
      final int y1 = (int) event.getY(1);

      if (event.getPointerId(0) == m_lastPointerId)
        return multiTouchEvent(event.getAction(), true, true,
            x0, y0, x1, y1, event);
      else
        return multiTouchEvent(event.getAction(), true, true, x1, y1, x0, y0, event);
    }
    }
  }

  /**
   * Called to initialize EGL. This function should not be called by the
   * inheriting activity, but can be overridden if needed.
   *
   * @return True if successful
   */
  protected boolean InitEGL()
  {
    mEglWrapper = EglWrapper.GetEgl(mLog);
    return mEglWrapper.InitEGL();
  }

  /**
   * Called to clean up m_egl. This function should not be called by the
   * inheriting activity, but can be overridden if needed.
   */
  protected boolean CleanupEGL()
  {
    return mEglWrapper != null && mEglWrapper.TerminateEGL();
  }

  protected boolean CreateSurfaceEGL()
  {
    if (!mEglWrapper.CreateSurfaceEGL(m_cachedSurfaceHolder))
      return false;

    m_surfaceHeight = mEglWrapper.GetSurfaceHeight();
    m_surfaceWidth = mEglWrapper.GetSurfaceWidth();
    return true;
  }

  /**
   * Destroys the EGLSurface used for rendering. This function should not be
   * called by the inheriting activity, but can be overridden if needed.
   */
  protected boolean DestroySurfaceEGL()
  {
    return mEglWrapper.DestroySurfaceEGL();
  }

  public boolean BindSurfaceAndContextEGL()
  {
    return mEglWrapper.Bind();
  }

  public boolean UnbindSurfaceAndContextEGL()
  {
    return mEglWrapper.Unbind();
  }

  public boolean SwapBuffersEGL()
  {
    return mEglWrapper.SwapBuffersEGL();
  }

  public int GetErrorEGL()
  {
    return mEglWrapper.GetErrorEGL();
  }

  public void OnRenderingInitialized()
  {
  }

  public void ReportUnsupported()
  {
    Log.i(TAG, "this phone GPU is unsupported");
  }
}
