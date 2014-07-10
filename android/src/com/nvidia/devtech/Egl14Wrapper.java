package com.nvidia.devtech;

import android.opengl.EGL14;
import android.opengl.EGLDisplay;
import android.opengl.EGLContext;
import android.opengl.EGLSurface;
import android.opengl.EGLConfig;

import android.view.SurfaceHolder;
import android.util.Log;

import com.mapswithme.util.log.Logger;

import java.util.Arrays;
import java.util.Comparator;

public class Egl14Wrapper extends BaseEglWrapper
{
  private static final String TAG = "Egl14Wrapper";
  private Logger mLog = null;

  private EGLDisplay mDisplay = EGL14.EGL_NO_DISPLAY;
  private EGLContext mContext = EGL14.EGL_NO_CONTEXT;
  private EGLSurface mSurface = EGL14.EGL_NO_SURFACE;
  private EGLConfig  mConfig  = null;

  private EGLConfig[] m_configs = new EGLConfig[40];

  private int m_choosenConfigIndex = 0;
  private int m_actualConfigsNumber[] = new int[] {0};

  private class EGLConfigComparator extends ConfigComparatorBase
                                    implements Comparator<EGLConfig>
  {
    EGLConfigComparator()
    {
      super(EGL14.EGL_NONE, EGL14.EGL_SLOW_CONFIG, EGL14.EGL_NON_CONFORMANT_CONFIG);
    }

    @Override
    public int compare(EGLConfig l, EGLConfig r)
    {
      final int [] value = new int[2];

      EGL14.eglGetConfigAttrib(mDisplay, l, EGL14.EGL_CONFIG_CAVEAT, value, 0);
      EGL14.eglGetConfigAttrib(mDisplay, r, EGL14.EGL_CONFIG_CAVEAT, value, 1);

      return CompareConfigs(value[1], value[0]);
    }
  };

  public Egl14Wrapper(Logger logger)
  {
    mLog = logger;
  }

  @Override
  public boolean InitEGL()
  {
    mDisplay = EGL14.eglGetDisplay(EGL14.EGL_DEFAULT_DISPLAY);
    if (mDisplay == EGL14.EGL_NO_DISPLAY)
    {
      LogIt("eglGetDisplay failed");
      return false;
    }

    int[] version = new int[2];
    if (!EGL14.eglInitialize(mDisplay, version, 0, version, 1))
    {
      LogIt("eglInitialize failed");
      return false;
    }

    if (version[0] != 1 && version[1] < 4)
    {
      LogIt("Incorrect EGL wrapper choosed");
      return false;
    }

    if (!EGL14.eglChooseConfig(mDisplay, GetConfigAttributes14(), 0, m_configs, 0, m_configs.length, m_actualConfigsNumber, 0))
    {
      LogEgl("eglChooseConfig failed with error ");
      return false;
    }

    if (m_actualConfigsNumber[0] == 0)
    {
      LogIt("eglChooseConfig returned zero configs");
      return false;
    }

    Arrays.sort(m_configs, 0, m_actualConfigsNumber[0], new EGLConfigComparator());
    m_choosenConfigIndex = 0;

    while (true)
    {
      mConfig = m_configs[m_choosenConfigIndex];

      // Debug print
      LogIt("Matched egl configs:");
      for (int i = 0; i < m_actualConfigsNumber[0]; ++i)
        LogIt((i == m_choosenConfigIndex ? "*" : " ") + i + ": " + eglConfigToString(m_configs[i]));

      mContext = EGL14.eglCreateContext(mDisplay, mConfig, EGL14.EGL_NO_CONTEXT, GetContextAttributes14(), 0);
      if (mContext == EGL14.EGL_NO_CONTEXT)
      {
        LogEgl("eglCreateContext failed with error ");
        m_choosenConfigIndex++;
      }
      else
        break;

      if (m_choosenConfigIndex == m_configs.length)
      {
        LogIt("No more configs left to choose");
        return false;
      }
    }

    SetIsInitialized(true);
    return true;
  }

  @Override
  public boolean TerminateEGL()
  {
    LogIt("CleanupEGL");

    if (!IsInitialized())
      return false;
    if (!DestroySurfaceEGL())
      return false;

    if (mDisplay != EGL14.EGL_NO_DISPLAY)
      EGL14.eglMakeCurrent(mDisplay, EGL14.EGL_NO_SURFACE, EGL14.EGL_NO_SURFACE, EGL14.EGL_NO_CONTEXT);

    if (mContext != EGL14.EGL_NO_CONTEXT)
      EGL14.eglDestroyContext(mDisplay, mContext);

    EGL14.eglTerminate(mDisplay);

    mDisplay = EGL14.EGL_NO_DISPLAY;
    mContext = EGL14.EGL_NO_CONTEXT;
    mConfig = null;

    SetIsInitialized(false);
    return true;
  }

  @Override
  public boolean CreateSurfaceEGL(SurfaceHolder holder)
  {
    if (holder == null)
    {
      LogIt("createEGLSurface failed, m_cachedSurfaceHolder is null");
      return false;
    }

    if (!IsInitialized())
      InitEGL();

    if (!IsInitialized())
    {
      LogIt("createEGLSurface failed, cannot initialize EGL");
      return false;
    }

    if (mDisplay == EGL14.EGL_NO_DISPLAY)
    {
      LogIt("createEGLSurface: display is null");
      return false;
    }
    else if (mConfig == null)
    {
      LogIt("createEGLSurface: config is null");
      return false;
    }

    int choosenSurfaceConfigIndex = m_choosenConfigIndex;

    while (true)
    {
      /// trying to create window surface with one of the EGL configs, recreating the m_eglConfig if necessary

      mSurface = EGL14.eglCreateWindowSurface(mDisplay, m_configs[choosenSurfaceConfigIndex], holder, GetSurfaceAttributes14(), 0);

      final boolean surfaceCreated = (mSurface != EGL14.EGL_NO_SURFACE);
      final boolean surfaceValidated = surfaceCreated ? ValidateSurfaceSize() : false;

      if (surfaceCreated && !surfaceValidated)
        EGL14.eglDestroySurface(mDisplay, mSurface);

      if (!surfaceCreated || !surfaceValidated)
      {
        LogIt("eglCreateWindowSurface failed for config : " + eglConfigToString(m_configs[choosenSurfaceConfigIndex]));
        choosenSurfaceConfigIndex += 1;
        if (choosenSurfaceConfigIndex == m_actualConfigsNumber[0])
        {
          mSurface = EGL14.EGL_NO_SURFACE;
          LogIt("no eglConfigs left");
          break;
        }
        else
          LogIt("trying : " + eglConfigToString(m_configs[choosenSurfaceConfigIndex]));
      }
      else
        break;
    }

    if ((choosenSurfaceConfigIndex != m_choosenConfigIndex) && (mSurface != EGL14.EGL_NO_SURFACE))
    {
      LogIt("window surface is created for eglConfig : " + eglConfigToString(m_configs[choosenSurfaceConfigIndex]));

      // unbinding context
      if (mDisplay != null)
        EGL14.eglMakeCurrent(mDisplay, EGL14.EGL_NO_SURFACE, EGL14.EGL_NO_SURFACE, EGL14.EGL_NO_CONTEXT);

      // destroying context
      if (mContext != null)
        EGL14.eglDestroyContext(mDisplay, mContext);

      // recreating context with same eglConfig as eglWindowSurface has
      mContext = EGL14.eglCreateContext(mDisplay, m_configs[choosenSurfaceConfigIndex], EGL14.EGL_NO_CONTEXT, GetContextAttributes14(), 0);
      if (mContext == EGL14.EGL_NO_CONTEXT)
      {
        LogEgl("context recreation failed with error ");
        return false;
      }

      m_choosenConfigIndex = choosenSurfaceConfigIndex;
      mConfig = m_configs[m_choosenConfigIndex];
    }

    return true;
  }

  @Override
  public boolean DestroySurfaceEGL()
  {
    if (mDisplay != EGL14.EGL_NO_DISPLAY && mSurface != EGL14.EGL_NO_SURFACE)
      EGL14.eglMakeCurrent(mDisplay, EGL14.EGL_NO_SURFACE, EGL14.EGL_NO_SURFACE, mContext);
    if (mSurface != EGL14.EGL_NO_SURFACE)
    {
      EGL14.eglDestroySurface(mDisplay, mSurface);
    }
    mSurface = EGL14.EGL_NO_SURFACE;

    return true;
  }

  @Override
  public boolean SwapBuffersEGL()
  {
    if (mSurface == EGL14.EGL_NO_SURFACE)
    {
      LogIt("Surface is NULL");
      return false;
    }

    if (mDisplay == EGL14.EGL_NO_DISPLAY)
    {
      LogIt("Display is NULL");
      return false;
    }

    if (!EGL14.eglSwapBuffers(mDisplay, mSurface))
    {
      LogEgl("eglSwapBuffer: ");
      return false;
    }
    return true;
  }

  @Override
  public boolean Bind()
  {
    if (mContext == EGL14.EGL_NO_CONTEXT)
    {
      LogIt("m_eglContext is NULL");
      return false;
    }
    else if (mSurface == EGL14.EGL_NO_SURFACE)
    {
      LogIt("m_eglSurface is NULL");
      return false;
    }
    else if (!EGL14.eglMakeCurrent(mDisplay, mSurface, mSurface, mContext))
    {
      LogEgl("eglMakeCurrent err: ");
      return false;
    }

    return true;
  }

  @Override
  public boolean Unbind()
  {
    if (mDisplay == EGL14.EGL_NO_DISPLAY)
      return false;

    return EGL14.eglMakeCurrent(mDisplay, EGL14.EGL_NO_SURFACE, EGL14.EGL_NO_SURFACE, EGL14.EGL_NO_CONTEXT);
  }

  @Override
  public int GetErrorEGL()
  {
    return EGL14.eglGetError();
  }

  @Override
  public int GetSurfaceWidth()
  {
    final int sizes[] = new int[1];
    EGL14.eglQuerySurface(mDisplay, mSurface, EGL14.EGL_WIDTH, sizes, 0);
    return  sizes[0];
  }

  @Override
  public int GetSurfaceHeight()
  {
    final int sizes[] = new int[1];
    EGL14.eglQuerySurface(mDisplay, mSurface, EGL14.EGL_HEIGHT, sizes, 0);
    return sizes[0];
  }

  private void LogIt(String message)
  {
    mLog.d(message);
  }

  private void LogEgl(String message)
  {
    mLog.d(message + EGL14.eglGetError());
  }

  String eglConfigToString(final EGLConfig config)
  {
    final int[] value = new int[11];
    EGL14.eglGetConfigAttrib(mDisplay, config, EGL14.EGL_RED_SIZE, value, 0);
    EGL14.eglGetConfigAttrib(mDisplay, config, EGL14.EGL_GREEN_SIZE, value, 1);
    EGL14.eglGetConfigAttrib(mDisplay, config, EGL14.EGL_BLUE_SIZE, value, 2);
    EGL14.eglGetConfigAttrib(mDisplay, config, EGL14.EGL_ALPHA_SIZE, value, 3);
    EGL14.eglGetConfigAttrib(mDisplay, config, EGL14.EGL_STENCIL_SIZE, value, 4);
    EGL14.eglGetConfigAttrib(mDisplay, config, EGL14.EGL_DEPTH_SIZE, value, 5);
    EGL14.eglGetConfigAttrib(mDisplay, config, EGL14.EGL_CONFIG_CAVEAT, value, 6);
    final String caveat = (value[6] == EGL14.EGL_NONE) ? "EGL_NONE" :
                          (value[6] == EGL14.EGL_SLOW_CONFIG) ? "EGL_SLOW_CONFIG" : "EGL_NON_CONFORMANT_CONFIG";
    EGL14.eglGetConfigAttrib(mDisplay, config, EGL14.EGL_BUFFER_SIZE, value, 7);
    EGL14.eglGetConfigAttrib(mDisplay, config, EGL14.EGL_LEVEL, value, 8);
    EGL14.eglGetConfigAttrib(mDisplay, config, EGL14.EGL_SAMPLE_BUFFERS, value, 9);
    EGL14.eglGetConfigAttrib(mDisplay, config, EGL14.EGL_SAMPLES, value, 10);

    return "R" + value[0] + "G" + value[1] + "B" + value[2] + "A" + value[3] +
        " Stencil:" + value[4] + " Depth:" + value[5] + " Caveat:" + caveat +
        " BufferSize:" + value[7]  + " Level:" + value[8] + " SampleBuffers:" + value[9] +
        " Samples:" + value[10];
  }
}
