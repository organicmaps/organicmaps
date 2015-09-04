package com.nvidia.devtech;

import android.opengl.EGL14;
import android.opengl.EGLDisplay;
import android.os.Build;
import android.view.SurfaceHolder;

import com.mapswithme.util.Utils;
import com.mapswithme.util.log.Logger;

abstract public class EglWrapper
{
  public abstract boolean InitEGL();

  public abstract boolean TerminateEGL();

  public abstract boolean CreateSurfaceEGL(SurfaceHolder holder);

  public abstract boolean DestroySurfaceEGL();

  public abstract boolean SwapBuffersEGL();

  public abstract int GetSurfaceWidth();

  public abstract int GetSurfaceHeight();

  public abstract boolean IsInitialized();

  public abstract boolean Bind();

  public abstract boolean Unbind();

  public abstract int GetErrorEGL();

  enum EglVersion
  {
    NonEgl,
    Egl,
    Egl14
  }

  static public EglWrapper GetEgl(Logger logger)
  {
    EglVersion version = QueryEglVersion();
    switch (version)
    {
    case Egl:
      return new Egl10Wrapper(logger);
    case Egl14:
      return new Egl14Wrapper(logger);
    }

    return null;
  }

  static private EglVersion QueryEglVersion()
  {
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.JELLY_BEAN_MR1)
      return EglVersion.Egl;

    EglVersion result = EglVersion.NonEgl;
    EGLDisplay display = EGL14.eglGetDisplay(EGL14.EGL_DEFAULT_DISPLAY);
    if (display != EGL14.EGL_NO_DISPLAY)
    {
      final int[] version = new int[2];
      if (EGL14.eglInitialize(display, version, 0, version, 1))
      {
        if (version[0] >= 1)
        {
          if (version[1] < 4)
            result = EglVersion.Egl;
          else
            result = EglVersion.Egl14;
        }
      }

      EGL14.eglTerminate(display);
    }

    return result;
  }
}
