package com.mapswithme.maps;

import java.io.File;

import com.mapswithme.maps.MainGLView;
import com.mapswithme.maps.R;
import com.mapswithme.maps.location.LocationService;

import android.app.Activity;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.os.Bundle;
import android.os.Environment;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.util.Log;

public class MWMActivity extends Activity
{
  private static String TAG = "MWMActivity";
  private final static String PACKAGE_NAME = "com.mapswithme.maps";
  
  private MainGLView m_view;

  private boolean m_locationEnabled = false;

  private String getAppBundlePath() throws NameNotFoundException
  {
    PackageManager packMgmr = getApplication().getPackageManager();
    ApplicationInfo appInfo = packMgmr.getApplicationInfo(PACKAGE_NAME, 0);
    return appInfo.sourceDir;
  }

  private String getDataStoragePath()
  {
    String storagePath = Environment.getExternalStorageDirectory().getAbsolutePath();
    return storagePath.concat(String.format("/%s/", PACKAGE_NAME));
  }
  
  @Override
  public void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);

    final String storagePath = getDataStoragePath();
    // create folder if it doesn't exist
    File f = new File(storagePath);
    f.mkdirs();

    try
    {
      nativeInit(getAppBundlePath(), storagePath);
    }
    catch (NameNotFoundException e)
    {
      e.printStackTrace();
    }
    
    m_view = new MainGLView(getApplication());

    setContentView(m_view);
  }

  @Override
  protected void onPause()
  {
    super.onPause();
    m_view.onPause();
    if (m_locationEnabled)
      LocationService.stop();
  }

  @Override
  protected void onResume()
  {
    super.onResume();
    m_view.onResume();
    if (m_locationEnabled)
      LocationService.start(this);
  }

  @Override
  public boolean onCreateOptionsMenu(Menu menu)
  {
    MenuInflater inflater = getMenuInflater();
    inflater.inflate(R.menu.main, menu);
    return true;
  }

  @Override
  public boolean onOptionsItemSelected(MenuItem item)
  {
    // Handle item selection
    switch (item.getItemId())
    {
    case R.id.my_position:
      if (m_locationEnabled)
        LocationService.stop();
      else
        LocationService.start(this);
      m_locationEnabled = !m_locationEnabled;
      return true;
    case R.id.download_maps:
      Intent intent = new Intent(this, DownloadUI.class);
      startActivity(intent);
      return true;
    default:
      return super.onOptionsItemSelected(item);
    }
  }

  static
  {
    System.loadLibrary("mapswithme");
  }
  
  private native void nativeInit(String apkPath, String storagePath);
}
