package com.mapswithme.maps;

import com.mapswithme.maps.GesturesProcessor;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.util.Log;
import android.view.MotionEvent;

public class MainGLView extends GLSurfaceView
{
  private static String TAG = "MainGLView";

  GesturesProcessor m_gestures;

  public MainGLView(Context context)
  {
    super(context);
    init();
  }

  private void init()
  {
    m_gestures = new GesturesProcessor();

    // Do native initialization with OpenGL.
    nativeInit();

    setRenderer(new MainRenderer());

    // When renderMode is RENDERMODE_WHEN_DIRTY, the renderer only rendered
    // when the surface is created, or when requestRender() is called.
    setRenderMode(RENDERMODE_WHEN_DIRTY);
  }

  @Override
  public boolean onTouchEvent(MotionEvent e)
  {
    m_gestures.onTouchEvent(e);
    return true; // if handled
  }

  private native void nativeInit();
}


class MainRenderer implements GLSurfaceView.Renderer
{
  private static String TAG = "MainGLView.MainRenderer";
  
  @Override
  public void onDrawFrame(GL10 gl)
  {
    Log.i(TAG, "onDrawFrame");
    nativeDraw();
  }

  @Override
  public void onSurfaceChanged(GL10 gl, int w, int h)
  {
    Log.i(TAG, "onSurfaceChanged");
    nativeResize(w, h);
  }

  @Override
  public void onSurfaceCreated(GL10 gl, EGLConfig config)
  {
    Log.i(TAG, "onSurfaceCreated");
    nativeInit();
  }
  
  private native void nativeInit();
  private native void nativeResize(int w, int h);
  private native void nativeDraw();
}
