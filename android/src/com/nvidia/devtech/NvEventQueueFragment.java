package com.nvidia.devtech;

import android.annotation.SuppressLint;
import android.os.Build;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceHolder.Callback;
import android.view.SurfaceView;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmFragment;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.StubLogger;

public abstract class NvEventQueueFragment extends BaseMwmFragment implements View.OnTouchListener, View.OnFocusChangeListener
{
  private static final String TAG = NvEventQueueFragment.class.getSimpleName();

  private final Logger mLog = StubLogger.get();

  private boolean mIsRenderingInitialized;
  private EglWrapper mEglWrapper;
  protected SurfaceHolder mCachedSurfaceHolder;
  protected int mSurfaceWidth;
  protected int mSurfaceHeight;

  private int mDisplayDensity;

  private boolean mIsNativeLaunched;

  private int mLastPointerId;

  public int getSurfaceWidth()
  {
    return mSurfaceWidth;
  }

  public int getSurfaceHeight()
  {
    return mSurfaceHeight;
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

  @Override
  public void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);

    final DisplayMetrics metrics = new DisplayMetrics();
    getActivity().getWindowManager().getDefaultDisplay().getMetrics(metrics);
    mDisplayDensity = metrics.densityDpi;
    mIsNativeLaunched = true;
    onCreateNative();
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB && getActivity().isChangingConfigurations())
      mIsRenderingInitialized = true;
  }

  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    return super.onCreateView(inflater, container, savedInstanceState);
  }

  @Override
  public void onViewCreated(View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    final SurfaceView surfaceView = (SurfaceView) view.findViewById(R.id.map_surfaceview);
    surfaceView.setOnFocusChangeListener(this);

    final SurfaceHolder holder = surfaceView.getHolder();
    holder.addCallback(new Callback()
    {
      @Override
      public void surfaceCreated(SurfaceHolder holder)
      {
        mCachedSurfaceHolder = holder;
        onSurfaceCreatedNative(mSurfaceWidth, mSurfaceHeight, mDisplayDensity);
      }

      @Override
      public void surfaceChanged(SurfaceHolder holder, int format, int width, int height)
      {
        mCachedSurfaceHolder = holder;
        mSurfaceWidth = width;
        mSurfaceHeight = height;
        onSurfaceChangedNative(mSurfaceWidth, mSurfaceHeight, mDisplayDensity);
        if (mIsRenderingInitialized)
          applyWidgetPivots();
      }

      @Override
      public void surfaceDestroyed(SurfaceHolder holder)
      {
        mCachedSurfaceHolder = null;
        onSurfaceDestroyedNative();
      }
    });
  }

  /**
   * Implement to position map widgets.
   */
  protected abstract void applyWidgetPivots();

  @Override
  public void onStart()
  {
    super.onStart();
    onStartNative();
  }

  @Override
  public void onResume()
  {
    super.onResume();
    onResumeNative();
    onFocusChangedNative(true);
  }

  @Override
  public void onFocusChange(View v, boolean hasFocus)
  {
    onFocusChangedNative(hasFocus);
  }

  @Override
  public void onPause()
  {
    super.onPause();
    onPauseNative();
    onFocusChangedNative(getActivity().hasWindowFocus());
  }

  @SuppressLint("NewApi")
  @Override
  public void onStop()
  {
    super.onStop();
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.HONEYCOMB || !getActivity().isChangingConfigurations())
    {
      // if configuration is changed - EGL shouldn't be reinitialized
      mIsRenderingInitialized = false;
      onStopNative();
    }
  }

  @Override
  public void onDestroy()
  {
    super.onDestroy();
    onStopNative();
    onDestroyNative();
    CleanupEGL();
  }

  @Override
  public boolean onTouch(View v, MotionEvent event)
  {
    // TODO refactor ?
    final int count = event.getPointerCount();

    if (!mIsNativeLaunched || count == 0)
      return false;

    switch (count)
    {
    case 1:
    {
      mLastPointerId = event.getPointerId(0);

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

      if (event.getPointerId(0) == mLastPointerId)
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
  @SuppressWarnings("UnusedDeclaration")
  protected boolean InitEGL()
  {
    mEglWrapper = EglWrapper.GetEgl(mLog);
    return mEglWrapper.InitEGL();
  }

  /**
   * Called to clean up EGL. This function should not be called by the
   * inheriting fragment, but can be overridden if needed.
   */
  protected boolean CleanupEGL()
  {
    return mEglWrapper != null && mEglWrapper.TerminateEGL();
  }

  @SuppressWarnings("UnusedDeclaration")
  protected boolean CreateSurfaceEGL()
  {
    if (!mEglWrapper.CreateSurfaceEGL(mCachedSurfaceHolder))
      return false;

    mSurfaceHeight = mEglWrapper.GetSurfaceHeight();
    mSurfaceWidth = mEglWrapper.GetSurfaceWidth();
    return true;
  }

  /**
   * Destroys the EGLSurface used for rendering. This function should not be
   * called by the inheriting activity, but can be overridden if needed.
   */
  @SuppressWarnings("UnusedDeclaration")
  protected boolean DestroySurfaceEGL()
  {
    return mEglWrapper.DestroySurfaceEGL();
  }

  @SuppressWarnings("UnusedDeclaration")
  public boolean BindSurfaceAndContextEGL()
  {
    return mEglWrapper.Bind();
  }

  @SuppressWarnings("UnusedDeclaration")
  public boolean UnbindSurfaceAndContextEGL()
  {
    return mEglWrapper.Unbind();
  }

  @SuppressWarnings("UnusedDeclaration")
  public boolean SwapBuffersEGL()
  {
    return mEglWrapper.SwapBuffersEGL();
  }

  @SuppressWarnings("UnusedDeclaration")
  public int GetErrorEGL()
  {
    return mEglWrapper.GetErrorEGL();
  }

  @SuppressWarnings("UnusedDeclaration")
  public void OnRenderingInitialized()
  {
    mIsRenderingInitialized = true;
    applyWidgetPivots();
  }

  public boolean isRenderingInitialized()
  {
    return mIsRenderingInitialized;
  }

  @SuppressWarnings("UnusedDeclaration")
  public void ReportUnsupported()
  {
    Log.i(TAG, "this phone GPU is unsupported");
  }

  @SuppressWarnings("UnusedDeclaration")
  public void finish()
  {
    if (isAdded())
      getActivity().finish();
  }
}
