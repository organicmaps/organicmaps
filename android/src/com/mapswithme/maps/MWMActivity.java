package com.mapswithme.maps;

import com.mapswithme.maps.MainGLView;

import android.app.Activity;
import android.os.Bundle;

public class MWMActivity extends Activity
{
  private MainGLView m_view;

  private String getResourcePath()
  {
    return new String("todo: get data path");    
  }
  
  @Override
  public void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);

    nativeInit(getResourcePath());
    
    m_view = new MainGLView(getApplication());

    setContentView(m_view);
  }

  @Override
  protected void onPause()
  {
    super.onPause();
    m_view.onPause();
  }

  @Override
  protected void onResume()
  {
    super.onResume();
    m_view.onResume();
  }

  static
  {
    System.loadLibrary("mapswithme");
  }
  
  private native void nativeInit(String path);
}
