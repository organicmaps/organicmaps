package com.nvidia.devtech;

import javax.microedition.khronos.egl.EGL10;
import android.opengl.EGL14;

abstract public class BaseEglWrapper extends EglWrapper
{
  private static final int EGL_RENDERABLE_TYPE = 0x3040;
  private static final int EGL_OPENGL_ES2_BIT = 0x0004;
  private static final int EGL_CONTEXT_CLIENT_VERSION = 0x3098;

  /** The number of bits requested for the red component */
  private static int redSize = 5;
  /** The number of bits requested for the green component */
  private static int greenSize = 6;
  /** The number of bits requested for the blue component */
  private static int blueSize = 5;
  /** The number of bits requested for the alpha component */
  private static int alphaSize = 0;
  /** The number of bits requested for the stencil component */
  private static int stencilSize = 0;
  /** The number of bits requested for the depth component */
  private static int depthSize = 16;

  private boolean mIsInitialized = false;

  public boolean IsInitialized()
  {
    return mIsInitialized;
  }

  protected void SetIsInitialized(boolean isInitialized)
  {
    mIsInitialized = isInitialized;
  }

  protected int[] GetConfigAttributes10()
  {
    final int[] configAttributes = new int[] {EGL10.EGL_RED_SIZE, redSize,
                                              EGL10.EGL_GREEN_SIZE, greenSize,
                                              EGL10.EGL_BLUE_SIZE, blueSize,
                                              EGL10.EGL_ALPHA_SIZE, alphaSize,
                                              EGL10.EGL_STENCIL_SIZE, stencilSize,
                                              EGL10.EGL_DEPTH_SIZE, depthSize,
                                              EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                                              EGL10.EGL_NONE };

    return configAttributes;
  }

  protected int[] GetSurfaceAttributes10()
  {
    return new int[] { EGL10.EGL_NONE };
  }

  protected int[] GetContextAttributes10()
  {
    return new int[] { EGL_CONTEXT_CLIENT_VERSION, 2, EGL10.EGL_NONE };
  }

  protected int[] GetConfigAttributes14()
  {
    final int[] configAttributes = new int[] {EGL14.EGL_RED_SIZE, redSize,
                                              EGL14.EGL_GREEN_SIZE, greenSize,
                                              EGL14.EGL_BLUE_SIZE, blueSize,
                                              EGL14.EGL_ALPHA_SIZE, alphaSize,
                                              EGL14.EGL_STENCIL_SIZE, stencilSize,
                                              EGL14.EGL_DEPTH_SIZE, depthSize,
                                              EGL14.EGL_RENDERABLE_TYPE, EGL14.EGL_OPENGL_ES2_BIT,
                                              EGL14.EGL_NONE };

    return configAttributes;
  }

  protected int[] GetSurfaceAttributes14()
  {
    return new int[] { EGL14.EGL_NONE };
  }

  protected int[] GetContextAttributes14()
  {
    return new int[] { EGL14.EGL_CONTEXT_CLIENT_VERSION, 2, EGL14.EGL_NONE };
  }

  protected boolean ValidateSurfaceSize()
  {
    return GetSurfaceWidth() * GetSurfaceWidth() != 0;
  }

  public class ConfigComparatorBase
  {
    private int EglNone;
    private int EglSlowConfig;
    private int EglNonConformantConfig;

    public ConfigComparatorBase(int eglNone, int eglSlow, int eglNonComformant)
    {
      EglNone = eglNone;
      EglSlowConfig = eglSlow;
      EglNonConformantConfig = eglNonComformant;
    }

    public int CompareConfigs(int rCav, int lCav)
    {
      if (lCav != rCav)
        return GetCoveatValue(lCav) - GetCoveatValue(rCav);

      return 0;
    }

    int GetCoveatValue(int cav)
    {
      if (cav == EglSlowConfig)
        return 1;
      else if (cav == EglNonConformantConfig)
        return 2;

      return 0;
    }
  }
}
