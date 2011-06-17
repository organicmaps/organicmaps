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

    setRenderer(new Renderer());

    // When renderMode is RENDERMODE_WHEN_DIRTY, the renderer only rendered
    // when the surface is created, or when requestRender() is called.
    setRenderMode(RENDERMODE_WHEN_DIRTY);
  }

  private static class Renderer implements GLSurfaceView.Renderer
  {
    @Override
    public void onDrawFrame(GL10 gl)
    {
      Log.i(TAG, "Renderer::onDrawFrame");
    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height)
    {
      Log.i(TAG, "Renderer::onSurfaceChanged");
    }

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config)
    {
      Log.i(TAG, "Renderer::onSurfaceCreated");
    }
  }

  @Override
  public boolean onTouchEvent(MotionEvent e)
  {
    m_gestures.onTouchEvent(e);
    return true; // if handled
  }

  private native void nativeInit();
}
