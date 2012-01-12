package com.mapswithme.maps;

import android.app.Activity;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.util.Log;

public class SmartGLActivity extends Activity
{
  private static String TAG = "SmartGLActivity";

  protected SmartGLSurfaceView m_view = null;

  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    Log.d(TAG, "onCreate");
    super.onCreate(savedInstanceState);

    setContentView(R.layout.map);
    m_view = (SmartGLSurfaceView)findViewById(R.id.map_smartglsurfaceview);
  }

  @Override
  protected void onRestart()
  {
    Log.d(TAG, "onRestart");
    super.onRestart();
  }

  @Override
  protected void onStart()
  {
    Log.d(TAG, "onStart");
    super.onStart();
  }

  @Override
  protected void onResume()
  {
    Log.d(TAG, "onResume");
    m_view.onResume();
    super.onResume();
  }

  @Override
  protected void onPause()
  {
    Log.d(TAG, "onPause");
    m_view.onPause();
    super.onPause();
  }

  @Override
  protected void onStop()
  {
    Log.d(TAG, "onStop");
    m_view.onStop();
    super.onStop();
  }

  @Override
  protected void onDestroy()
  {
    Log.d(TAG, "onDestroy");
    super.onDestroy();
  }
}
