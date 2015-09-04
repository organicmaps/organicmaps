package com.nvidia.devtech;

import android.os.Build;
import android.view.SurfaceHolder;

import com.mapswithme.util.log.Logger;

import java.util.Arrays;
import java.util.Comparator;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.egl.EGLSurface;

public class Egl10Wrapper extends BaseEglWrapper
{
  private static final String TAG = "Egl10Wrapper";

  final private EGL10 mEgl = (EGL10) EGLContext.getEGL();

  private EGLDisplay mDisplay = EGL10.EGL_NO_DISPLAY;
  private EGLContext mContext = EGL10.EGL_NO_CONTEXT;
  private EGLSurface mSurface = EGL10.EGL_NO_SURFACE;
  private EGLConfig mConfig = null;

  private EGLConfig[] mConfigs = new EGLConfig[40];

  private int mChoosenConfigIndex = 0;
  private int mActualConfigsNumber[] = new int[]{0};

  private Logger mLog = null;

  public Egl10Wrapper(Logger logger)
  {
    mLog = logger;
  }

  private class EGLConfigComparator extends ConfigComparatorBase
      implements Comparator<EGLConfig>
  {
    EGLConfigComparator()
    {
      super(EGL10.EGL_NONE, EGL10.EGL_SLOW_CONFIG, EGL10.EGL_NON_CONFORMANT_CONFIG);
    }

    @Override
    public int compare(EGLConfig l, EGLConfig r)
    {
      final int[] value = new int[1];

      /// splitting by EGL_CONFIG_CAVEAT,
      /// firstly selecting EGL_NONE, then EGL_SLOW_CONFIG
      /// and then EGL_NON_CONFORMANT_CONFIG
      mEgl.eglGetConfigAttrib(mDisplay, l, EGL10.EGL_CONFIG_CAVEAT, value);
      final int lcav = value[0];

      mEgl.eglGetConfigAttrib(mDisplay, r, EGL10.EGL_CONFIG_CAVEAT, value);
      final int rcav = value[0];

      return CompareConfigs(rcav, lcav);
    }
  }

  ;

  @Override
  public boolean InitEGL()
  {
    mDisplay = mEgl.eglGetDisplay(EGL10.EGL_DEFAULT_DISPLAY);
    if (mDisplay == EGL10.EGL_NO_DISPLAY)
    {
      LogIt("eglGetDisplay failed");
      return false;
    }

    int[] version = new int[2];
    if (!mEgl.eglInitialize(mDisplay, version))
    {
      LogIt("eglInitialize failed");
      return false;
    }

    if (version[0] != 1 && version[1] >= 4 && Build.VERSION.SDK_INT < Build.VERSION_CODES.JELLY_BEAN_MR1)
    {
      LogIt("Incorrect EGL wrapper choosed");
      return false;
    }

    if (!mEgl.eglChooseConfig(mDisplay, GetConfigAttributes10(), mConfigs, mConfigs.length, mActualConfigsNumber))
    {
      LogEgl("eglChooseConfig failed with error ");
      return false;
    }

    if (mActualConfigsNumber[0] == 0)
    {
      LogIt("eglChooseConfig returned zero configs");
      return false;
    }

    Arrays.sort(mConfigs, 0, mActualConfigsNumber[0], new EGLConfigComparator());
    mChoosenConfigIndex = 0;

    while (true)
    {
      mConfig = mConfigs[mChoosenConfigIndex];

      // Debug print
      LogIt("Matched egl configs:");
      for (int i = 0; i < mActualConfigsNumber[0]; ++i)
        LogIt((i == mChoosenConfigIndex ? "*" : " ") + i + ": " + eglConfigToString(mConfigs[i]));

      mContext = mEgl.eglCreateContext(mDisplay, mConfig, EGL10.EGL_NO_CONTEXT, GetContextAttributes10());
      if (mContext == EGL10.EGL_NO_CONTEXT)
      {
        LogEgl("eglCreateContext failed with error ");
        mChoosenConfigIndex++;
      }
      else
        break;

      if (mChoosenConfigIndex == mConfigs.length)
      {
        LogIt("No more configs left to choose");
        return false;
      }
    }

    return true;
  }

  @Override
  public boolean IsInitialized()
  {
    return mContext != EGL10.EGL_NO_CONTEXT;
  }

  @Override
  public boolean TerminateEGL()
  {
    LogIt("CleanupEGL");

    if (!IsInitialized())
      return false;

    if (!DestroySurfaceEGL())
      return false;

    if (mDisplay != EGL10.EGL_NO_DISPLAY)
      mEgl.eglMakeCurrent(mDisplay, EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_CONTEXT);

    if (mContext != EGL10.EGL_NO_CONTEXT)
      mEgl.eglDestroyContext(mDisplay, mContext);

    mEgl.eglTerminate(mDisplay);

    mDisplay = EGL10.EGL_NO_DISPLAY;
    mContext = EGL10.EGL_NO_CONTEXT;
    mConfig = null;

    return true;
  }

  @Override
  public boolean CreateSurfaceEGL(SurfaceHolder holder)
  {
    if (holder == null)
    {
      LogIt("createEGLSurface failed, mCachedSurfaceHolder is null");
      return false;
    }

    if (!IsInitialized())
      InitEGL();

    if (!IsInitialized())
    {
      LogIt("createEGLSurface failed, cannot initialize EGL");
      return false;
    }

    if (mDisplay == EGL10.EGL_NO_DISPLAY)
    {
      LogIt("createEGLSurface: display is null");
      return false;
    }
    else if (mConfig == null)
    {
      LogIt("createEGLSurface: config is null");
      return false;
    }

    int choosenSurfaceConfigIndex = mChoosenConfigIndex;

    while (true)
    {
      /// trying to create window surface with one of the EGL configs, recreating the m_eglConfig if necessary

      mSurface = mEgl.eglCreateWindowSurface(mDisplay, mConfigs[choosenSurfaceConfigIndex], holder, GetSurfaceAttributes10());

      final boolean surfaceCreated = (mSurface != EGL10.EGL_NO_SURFACE);
      final boolean surfaceValidated = surfaceCreated ? ValidateSurfaceSize() : false;

      if (surfaceCreated && !surfaceValidated)
        mEgl.eglDestroySurface(mDisplay, mSurface);

      if (!surfaceCreated || !surfaceValidated)
      {
        LogIt("eglCreateWindowSurface failed for config : " + eglConfigToString(mConfigs[choosenSurfaceConfigIndex]));
        choosenSurfaceConfigIndex += 1;
        if (choosenSurfaceConfigIndex == mActualConfigsNumber[0])
        {
          mSurface = EGL10.EGL_NO_SURFACE;
          LogIt("no eglConfigs left");
          break;
        }
        else
          LogIt("trying : " + eglConfigToString(mConfigs[choosenSurfaceConfigIndex]));
      }
      else
        break;
    }

    if ((choosenSurfaceConfigIndex != mChoosenConfigIndex) && (mSurface != null))
    {
      LogIt("window surface is created for eglConfig : " + eglConfigToString(mConfigs[choosenSurfaceConfigIndex]));

      // unbinding context
      if (mDisplay != null)
        mEgl.eglMakeCurrent(mDisplay, EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_CONTEXT);

      // destroying context
      if (mContext != null)
        mEgl.eglDestroyContext(mDisplay, mContext);

      // recreating context with same eglConfig as eglWindowSurface has
      mContext = mEgl.eglCreateContext(mDisplay, mConfigs[choosenSurfaceConfigIndex], EGL10.EGL_NO_CONTEXT, GetContextAttributes10());
      if (mContext == EGL10.EGL_NO_CONTEXT)
      {
        LogEgl("context recreation failed with error ");
        return false;
      }

      mChoosenConfigIndex = choosenSurfaceConfigIndex;
      mConfig = mConfigs[mChoosenConfigIndex];
    }

    return true;
  }

  @Override
  public boolean DestroySurfaceEGL()
  {
    if (mDisplay != EGL10.EGL_NO_DISPLAY && mSurface != EGL10.EGL_NO_SURFACE)
      mEgl.eglMakeCurrent(mDisplay, EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_SURFACE, mContext);
    if (mSurface != EGL10.EGL_NO_SURFACE)
      mEgl.eglDestroySurface(mDisplay, mSurface);
    mSurface = EGL10.EGL_NO_SURFACE;

    return true;
  }

  @Override
  public boolean SwapBuffersEGL()
  {
    if (mSurface == EGL10.EGL_NO_SURFACE)
    {
      LogIt("Surface is NULL");
      return false;
    }

    if (mDisplay == EGL10.EGL_NO_DISPLAY)
    {
      LogIt("Display is NULL");
      return false;
    }

    if (!mEgl.eglSwapBuffers(mDisplay, mSurface))
    {
      LogEgl("eglSwapBuffer: ");
      return false;
    }
    return true;
  }

  @Override
  public int GetSurfaceWidth()
  {
    final int sizes[] = new int[1];
    mEgl.eglQuerySurface(mDisplay, mSurface, EGL10.EGL_WIDTH, sizes);
    return sizes[0];
  }

  @Override
  public int GetSurfaceHeight()
  {
    final int sizes[] = new int[1];
    mEgl.eglQuerySurface(mDisplay, mSurface, EGL10.EGL_HEIGHT, sizes);
    return sizes[0];
  }

  @Override
  public boolean Bind()
  {
    if (mContext == EGL10.EGL_NO_CONTEXT)
    {
      LogIt("m_eglContext is NULL");
      return false;
    }
    else if (mSurface == EGL10.EGL_NO_SURFACE)
    {
      LogIt("m_eglSurface is NULL");
      return false;
    }
    else if (!mEgl.eglMakeCurrent(mDisplay, mSurface, mSurface, mContext))
    {
      LogEgl("eglMakeCurrent err: ");
      return false;
    }

    return true;
  }

  @Override
  public boolean Unbind()
  {
    if (mDisplay == EGL10.EGL_NO_DISPLAY)
      return false;

    return mEgl.eglMakeCurrent(mDisplay, EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_CONTEXT);
  }

  @Override
  public int GetErrorEGL()
  {
    return mEgl.eglGetError();
  }

  private void LogIt(String message)
  {
    mLog.d(TAG, message);
  }

  private void LogEgl(String message)
  {
    mLog.d(TAG, message + mEgl.eglGetError());
  }

  String eglConfigToString(final EGLConfig config)
  {
    final int[] value = new int[1];
    mEgl.eglGetConfigAttrib(mDisplay, config, EGL10.EGL_RED_SIZE, value);
    final int red = value[0];
    mEgl.eglGetConfigAttrib(mDisplay, config, EGL10.EGL_GREEN_SIZE, value);
    final int green = value[0];
    mEgl.eglGetConfigAttrib(mDisplay, config, EGL10.EGL_BLUE_SIZE, value);
    final int blue = value[0];
    mEgl.eglGetConfigAttrib(mDisplay, config, EGL10.EGL_ALPHA_SIZE, value);
    final int alpha = value[0];
    mEgl.eglGetConfigAttrib(mDisplay, config, EGL10.EGL_STENCIL_SIZE, value);
    final int stencil = value[0];
    mEgl.eglGetConfigAttrib(mDisplay, config, EGL10.EGL_DEPTH_SIZE, value);
    final int depth = value[0];
    mEgl.eglGetConfigAttrib(mDisplay, config, EGL10.EGL_CONFIG_CAVEAT, value);
    final String caveat = (value[0] == EGL10.EGL_NONE) ? "EGL_NONE" :
        (value[0] == EGL10.EGL_SLOW_CONFIG) ? "EGL_SLOW_CONFIG" : "EGL_NON_CONFORMANT_CONFIG";
    mEgl.eglGetConfigAttrib(mDisplay, config, EGL10.EGL_BUFFER_SIZE, value);
    final int buffer = value[0];
    mEgl.eglGetConfigAttrib(mDisplay, config, EGL10.EGL_LEVEL, value);
    final int level = value[0];
    mEgl.eglGetConfigAttrib(mDisplay, config, EGL10.EGL_SAMPLE_BUFFERS, value);
    final int sampleBuffers = value[0];
    mEgl.eglGetConfigAttrib(mDisplay, config, EGL10.EGL_SAMPLES, value);
    final int samples = value[0];

    return "R" + red + "G" + green + "B" + blue + "A" + alpha +
        " Stencil:" + stencil + " Depth:" + depth + " Caveat:" + caveat +
        " BufferSize:" + buffer + " Level:" + level + " SampleBuffers:" + sampleBuffers +
        " Samples:" + samples;
  }
}
