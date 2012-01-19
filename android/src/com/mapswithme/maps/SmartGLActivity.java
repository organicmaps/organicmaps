package com.mapswithme.maps;

import android.app.Activity;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.view.WindowManager;

public class SmartGLActivity extends Activity
{
  private static String TAG = "SmartGLActivity";

  protected SmartGLSurfaceView m_view = null;

  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    Log.d(TAG, "onCreate");
    super.onCreate(savedInstanceState);

    if (!Build.MODEL.equals("Kindle Fire"))
      getWindow().clearFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);

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
