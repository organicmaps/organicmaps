package com.mapswithme.maps;

import com.mapswithme.maps.MainGLView;
import com.mapswithme.maps.R;

import android.app.Activity;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;

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

  @Override
  public boolean onCreateOptionsMenu(Menu menu) {
      MenuInflater inflater = getMenuInflater();
      inflater.inflate(R.menu.main, menu);
      return true;
  }

  @Override
  public boolean onOptionsItemSelected(MenuItem item) {
      // Handle item selection
      switch (item.getItemId()) {
      case R.id.my_position:
          return true;
      case R.id.download_maps:
          return true;
      default:
          return super.onOptionsItemSelected(item);
      }
  }

  static
  {
    System.loadLibrary("mapswithme");
  }
  
  private native void nativeInit(String path);
}
