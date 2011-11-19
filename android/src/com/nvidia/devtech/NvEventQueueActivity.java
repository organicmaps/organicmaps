//----------------------------------------------------------------------------------
// File:            libs\src\com\nvidia\devtech\NvEventQueueActivity.java
// Samples Version: NVIDIA Android Lifecycle samples 1_0beta 
// Email:           tegradev@nvidia.com
// Web:             http://developer.nvidia.com/category/zone/mobile-development
//
// Copyright 2009-2011 NVIDIA® Corporation 
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

import android.app.Activity;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.os.Bundle;
import android.view.KeyEvent;
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

/**
A base class used to provide a native-code event-loop interface to an
application.  This class is designed to be subclassed by the application
with very little need to extend the Java.  Paired with its native static-link
library, libnv_event.a, this package makes it possible for native applciations
to avoid any direct use of Java code.  In addition, input and other events are
automatically queued and provided to the application in native code via a
classic event queue-like API.  EGL functionality such as bind/unbind and swap
are also made available to the native code for ease of application porting.
Please see the external SDK documentation for an introduction to the use of
this class and its paired native library.
*/
public abstract class NvEventQueueActivity
	extends Activity 
    implements SensorEventListener
{
  protected boolean wantsMultitouch = true;

  protected boolean wantsAccelerometer = false;
  protected SensorManager mSensorManager = null;
  protected int mSensorDelay = SensorManager.SENSOR_DELAY_UI; //other options: SensorManager.SENSOR_DELAY_FASTEST, SensorManager.SENSOR_DELAY_NORMAL and SensorManager.SENSOR_DELAY_UI

  protected SurfaceView view3d = null;
	
  //private static final int EGL_RENDERABLE_TYPE = 0x3040;
  //private static final int EGL_OPENGL_ES_BIT = 0x0001;
  //private static final int EGL_OPENGL_ES2_BIT = 0x0004;
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

  /* *
   * Helper function to select fixed window size.
   * */ 
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
       
  /**
   * Function called when app requests accelerometer events.
   * Applications need/should NOT overide this function - it will provide
   * accelerometer events into the event queue that is accessible
   * via the calls in nv_event.h
   * 
   * @param values0: values[0] passed to onSensorChanged(). For accelerometer: Acceleration minus Gx on the x-axis.
   * @param values1: values[1] passed to onSensorChanged(). For accelerometer: Acceleration minus Gy on the y-axis.
   * @param values2: values[2] passed to onSensorChanged(). For accelerometer: Acceleration minus Gz on the z-axis.
   * @return True if the event was handled.
   */
  public native boolean accelerometerEvent(float values0, float values1, float values2);
    
  /**
   * The following indented function implementations are defined in libnvevent.a
   * The application does not and should not overide this; nv_event handles this internally
   * And remaps as needed into the native calls exposed by nv_event.h
   */
     
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

   protected native boolean postUserEvent(int u0, int u1, int u2, int u3, 
		   								  boolean blocking);
     
   public native boolean touchEvent(int action, int x, int y, 
		   							MotionEvent event);
   
   public native boolean multiTouchEvent(int action, 
		   		 						 boolean hasFirst,
		   		 						 boolean hasSecond, 
		   	                             int x0, int y0, int x1, int y1, 
		   	                             MotionEvent event);
   
   public native boolean keyEvent(int action, int keycode, int unicodeChar, KeyEvent event);
   
  /**
   * END indented block, see in comment at top of block
   */

   @Override
   public void onCreate(Bundle savedInstanceState)
   {
     System.out.println("**** onCreate");
     super.onCreate(savedInstanceState);
        
     if( wantsAccelerometer && (mSensorManager == null) )
       mSensorManager = (SensorManager) getSystemService(SENSOR_SERVICE);

     // If the app provides a non-null view3d, then we assume that the
     // app is doing its own custom layout, and we do not create a view
     // We use the supplied one.  Otherwise, we create it for them.
     if (view3d == null)
     {
       System.out.println("**** onCreate: Creating default view");        
       view3d = new SurfaceView(this);
       setContentView(view3d);
     }
     else
       System.out.println("**** onCreate: App specified custom view");        		
     
     SurfaceHolder holder = view3d.getHolder();
     holder.setType(SurfaceHolder.SURFACE_TYPE_GPU);

     holder.addCallback(new Callback()
     {
       // @Override
       public void surfaceCreated(SurfaceHolder holder)
       {
         System.out.println("systemInit.surfaceCreated");
         cachedSurfaceHolder = holder;
				
         if (fixedWidth!=0 && fixedHeight!=0)
         {
           System.out.println("Setting fixed window size");
           holder.setFixedSize(fixedWidth, fixedHeight);
         }

         onSurfaceCreatedNative(surfaceWidth, surfaceHeight);
       }

       // @Override
       public void surfaceChanged(SurfaceHolder holder, int format,
                    			  int width, int height)
       {
         cachedSurfaceHolder = holder;
         System.out.println("Surface changed: " + width + ", " + height);
         surfaceWidth = width;
         surfaceHeight = height;
         onSurfaceChangedNative(surfaceWidth, surfaceHeight);
       }

       // @Override
       public void surfaceDestroyed(SurfaceHolder holder)
       {
    	 cachedSurfaceHolder = null;
    	 System.out.println("systemInit.surfaceDestroyed");
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
      {
		if(mSensorManager != null)
          mSensorManager.registerListener(this, 
										  mSensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER), 
										  mSensorDelay);
		onResumeNative();
      }
    }
    
    @Override
    public void onLowMemory () 
    {
      System.out.println("**** onLowMemory");
      super.onLowMemory();
    }

    @Override
    public void onWindowFocusChanged (boolean hasFocus) 
    {
      System.out.println("**** onWindowFocusChanged (" + ((hasFocus == true) ? "TRUE" : "FALSE") + ")");
      if (nativeLaunched)
	    onFocusChangedNative(hasFocus);
      super.onWindowFocusChanged(hasFocus);
    }

    @Override
    protected void onSaveInstanceState (Bundle outState) 
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
      {
        if(mSensorManager != null)
          mSensorManager.unregisterListener(this);
	        
	    onStopNative();
	  }
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

    /**
     * Implementation function: defined in libnvevent.a
     * The application does not and should not overide this; nv_event handles this internally
     * And remaps as needed into the native calls exposed by nv_event.h
     */
	public void onAccuracyChanged(Sensor sensor, int accuracy) {
	  // Auto-generated method stub
	}

    /**
     * Implementation function: defined in libnvevent.a
     * The application does not and should not overide this; nv_event handles this internally
     * And remaps as needed into the native calls exposed by nv_event.h
     */
	public void onSensorChanged(SensorEvent event) {
	  // Auto-generated method stub
	  if (nativeLaunched && (event.sensor.getType() == Sensor.TYPE_ACCELEROMETER))
	    accelerometerEvent(event.values[0], event.values[1], event.values[2]);
	}
    
    /**
     * Implementation function: defined in libnvevent.a
     * The application does not and should not overide this; nv_event handles this internally
     * And remaps as needed into the native calls exposed by nv_event.h
     */
    @Override
    public boolean onTouchEvent(MotionEvent event)
    {
      boolean ret = super.onTouchEvent(event);
      if (nativeLaunched && !ret)
      {
        if (wantsMultitouch)
        {
          boolean hasFirst = false;
          boolean hasSecond = false;

          int x1 = 0, y1 = 0, x2 = 0, y2 = 0;
          // marshal up the data.
          int numEvents = event.getPointerCount();
          for (int i=0; i<numEvents; i++)
          {
            // only use pointers 0 and 1...
            int index = event.getPointerId(i);
            if (index < 2)
            {
              if (index == 0)
              {
                hasFirst = true;
                x1 = (int)event.getX(i);
                y1 = (int)event.getY(i);
              }
              else if (index == 1)
              {
                hasSecond = true;
                x2 = (int)event.getX(i);
                y2 = (int)event.getY(i);
              }
/*              if (count == 0)
              {
                x1 = (int)event.getX(i);
                y1 = (int)event.getY(i);
                count++;
              }
              else if (count == 1)
              {
                x2 = (int)event.getX(i);
                y2 = (int)event.getY(i);
                count++;
              }*/
            }
          }
          ret = multiTouchEvent(event.getAction(), hasFirst, hasSecond, x1, y1, x2, y2, event);
        }
        else // old style input.*/
        {
          ret = touchEvent(event.getAction(), (int)event.getX(), (int)event.getY(), event);
        }
      }
      return ret;
    }

    /**
     * Implementation function: defined in libnvevent.a
     * The application does not and should not overide this; nv_event handles this internally
     * And remaps as needed into the native calls exposed by nv_event.h
     */
    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event)
    {
      if (nativeLaunched && keyEvent(event.getAction(), keyCode, event.getUnicodeChar(), event))
        return true;
      return super.onKeyDown(keyCode, event);
    }
 
    /**
     * Implementation function: defined in libnvevent.a
     * The application does not and should not overide this; nv_event handles this internally
     * And remaps as needed into the native calls exposed by nv_event.h
     */
    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event)
    {
      if (nativeLaunched && keyEvent(event.getAction(), keyCode, event.getUnicodeChar(), event))
        return true;
      return super.onKeyUp(keyCode, event);
    }

    /** The number of bits requested for the red component */
    protected int redSize     = 8;
    /** The number of bits requested for the green component */
    protected int greenSize   = 8;
    /** The number of bits requested for the blue component */
    protected int blueSize    = 8;
    /** The number of bits requested for the alpha component */
    protected int alphaSize   = 8;
    /** The number of bits requested for the stencil component */
    protected int stencilSize = 8;
    /** The number of bits requested for the depth component */
    protected int depthSize   = 24;

    /** Attributes used when selecting the EGLConfig */
    protected int[] configAttrs = null;
    /** Attributes used when creating the context */
    protected int[] contextAttrs = null;

    /**
     * Called to initialize EGL. This function should not be called by the inheriting
     * activity, but can be overridden if needed.
     * 
     * @return True if successful
     */
    protected boolean InitEGL()
    {
      if (configAttrs == null)
        configAttrs = new int[] {EGL10.EGL_NONE};
      int[] oldConf = configAttrs;
        
      configAttrs = new int[3 + oldConf.length-1];
      int i = 0;
      for (i = 0; i < oldConf.length-1; i++)
        configAttrs[i] = oldConf[i];
      configAttrs[i++] = EGL10.EGL_RENDERABLE_TYPE;
      configAttrs[i++] = 1;
      configAttrs[i++] = EGL10.EGL_NONE;

      contextAttrs = new int[]
      {
        EGL_CONTEXT_CLIENT_VERSION, 1,
        EGL10.EGL_NONE
      };
        
      int[] oldConfES = configAttrs;
        
      configAttrs = new int[13 + oldConfES.length-1];
        
      for (i = 0; i < oldConfES.length-1; i++)
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
      System.out.println("eglDisplay: " + eglDisplay + ", err: " + egl.eglGetError());
        
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
      egl.eglChooseConfig(eglDisplay, configAttrs, config, config.length, num_configs);
      System.out.println("eglChooseConfig err: " + egl.eglGetError());

      int score = 1<<24; // to make sure even worst score is better than this, like 8888 when request 565...
      int val[] = new int[1];
      for (i = 0; i < num_configs[0]; i++)
      {
        boolean cont = true;
        int currScore = 0;
        int r, g, b, a, d, s;
        for (int j = 0; j < (oldConf.length-1)>>1; j++)
        {
          egl.eglGetConfigAttrib(eglDisplay, config[i], configAttrs[j*2], val);
          if ((val[0] & configAttrs[j*2+1]) != configAttrs[j*2+1])
          {
            cont = false; // Doesn't match the "must have" configs
            break;
          }
        }
        if (!cont)
          continue;
        egl.eglGetConfigAttrib(eglDisplay, config[i], EGL10.EGL_RED_SIZE, val); r = val[0];
        egl.eglGetConfigAttrib(eglDisplay, config[i], EGL10.EGL_GREEN_SIZE, val); g = val[0];
        egl.eglGetConfigAttrib(eglDisplay, config[i], EGL10.EGL_BLUE_SIZE, val); b = val[0];
        egl.eglGetConfigAttrib(eglDisplay, config[i], EGL10.EGL_ALPHA_SIZE, val); a = val[0];
        egl.eglGetConfigAttrib(eglDisplay, config[i], EGL10.EGL_DEPTH_SIZE, val); d = val[0];
        egl.eglGetConfigAttrib(eglDisplay, config[i], EGL10.EGL_STENCIL_SIZE, val); s = val[0];
        
        System.out.println(">>> EGL Config ["+i+"] R"+r+"G"+g+"B"+b+"A"+a+" D"+d+"S"+s);

        currScore = (Math.abs(r - redSize) + Math.abs(g - greenSize) + Math.abs(b - blueSize) + Math.abs(a - alphaSize)) << 16;
        currScore += Math.abs(d - depthSize) << 8;
        currScore += Math.abs(s - stencilSize);
        
        if (currScore < score)
        {
          System.out.println("--------------------------");
          System.out.println("New config chosen: " + i);
          for (int j = 0; j < (configAttrs.length-1)>>1; j++)
          {
            egl.eglGetConfigAttrib(eglDisplay, config[i], configAttrs[j*2], val);
            if (val[0] >= configAttrs[j*2+1])
              System.out.println("setting " + j + ", matches: " + val[0]);
          }

          score = currScore;
          eglConfig = config[i];
        }
      }
        
      eglContext = egl.eglCreateContext(eglDisplay, eglConfig, EGL10.EGL_NO_CONTEXT, contextAttrs);
      System.out.println("eglCreateContext: " + egl.eglGetError());

      gl = (GL11) eglContext.getGL();
        
      eglInitialized = true;
        
      return true;
    }

    /**
     * Called to clean up egl. This function should not be called by the inheriting
     * activity, but can be overridden if needed.
     */
    protected boolean CleanupEGL()
    {
      System.out.println("cleanupEGL");
		
      if (!eglInitialized)
        return false;
		
      if (!DestroySurfaceEGL())
        return false;
			
      if (eglDisplay != null)
        egl.eglMakeCurrent(eglDisplay, EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_CONTEXT);
      if (eglContext != null) {
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

    /**
     * Called to create the EGLSurface to be used for rendering. This function should not be called by the inheriting
     * activity, but can be overridden if needed.
     * 
     * @param surface The SurfaceHolder that holds the surface that we are going to render to.
     * @return True if successful
     */
    protected boolean CreateSurfaceEGL()
    {
      if (cachedSurfaceHolder == null)
      {
        System.out.println("createEGLSurface failed, cachedSurfaceHolder is null");
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
      }
      else if (eglConfig == null)
      {
        System.out.println("createEGLSurface: config is null");
        return false;
      }
      eglSurface = egl.eglCreateWindowSurface(eglDisplay, eglConfig, cachedSurfaceHolder, null);
      System.out.println("eglSurface: " + eglSurface + ", err: " + egl.eglGetError());
      int sizes[] = new int[1];
        
      egl.eglQuerySurface(eglDisplay, eglSurface, EGL10.EGL_WIDTH, sizes);
      surfaceWidth = sizes[0];
      egl.eglQuerySurface(eglDisplay, eglSurface, EGL10.EGL_HEIGHT, sizes);
      surfaceHeight = sizes[0];
      
      return true;
    }

    /**
     * Destroys the EGLSurface used for rendering. This function should not be called by the inheriting
     * activity, but can be overridden if needed.
     */
    protected boolean DestroySurfaceEGL()
    {
      if (eglDisplay != null && eglSurface != null)
        egl.eglMakeCurrent(eglDisplay, EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_SURFACE, eglContext);
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
      }
      else if (eglSurface == null)
      {
        System.out.println("eglSurface is NULL");
        return false;
      }
      else if (!egl.eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext))
      {
        System.out.println("eglMakeCurrent err: " + egl.eglGetError());
        return false;
      }

//        nvAcquireTimeExtension(); 
      return true;
    }

    /**
     * Implementation function: 
     * The application does not and should not overide or call this directly
     * Instead, the application should call NVEventEGLUnmakeCurrent(),
     * which is declared in nv_event.h
     */
    public boolean UnbindSurfaceAndContextEGL()
    {
      System.out.println("UnbindSurfaceAndContextEGL");
      if (eglDisplay == null)
      {
        System.out.println("UnbindSurfaceAndContextEGL: display is null");
        return false;
      }
		
      if (!egl.eglMakeCurrent(eglDisplay, EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_CONTEXT))
      {
        System.out.println("egl(Un)MakeCurrent err: " + egl.eglGetError());
        return false;
      }
	    
      return true;
    }

    public boolean SwapBuffersEGL()
    {
      //long stopTime;
      //long startTime = nvGetSystemTime();
      if (eglSurface == null)
      {
        System.out.println("eglSurface is NULL");
        return false;
      }
      else if (!egl.eglSwapBuffers(eglDisplay, eglSurface))
      {
        System.out.println("eglSwapBufferrr: " + egl.eglGetError());
        return false;
      }
      //stopTime = nvGetSystemTime();
      //String s = String.format("%d ms in eglSwapBuffers", (int)(stopTime - startTime));
      //Log.v("EventAccelerometer", s);
	    
      return true;
    }    
    
    public int GetErrorEGL()
    {
      return egl.eglGetError();
    }
}
