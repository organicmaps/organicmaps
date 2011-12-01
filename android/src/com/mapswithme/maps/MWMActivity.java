package com.mapswithme.maps;

import java.io.File;

import com.mapswithme.maps.R;
import com.mapswithme.maps.location.LocationService;
import com.nvidia.devtech.NvEventQueueActivity;

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

public class MWMActivity extends NvEventQueueActivity implements LocationService.Observer
{
  VideoTimer m_timer;
  
  private static String TAG = "MWMActivity";
  private final static String PACKAGE_NAME = "com.mapswithme.maps";
  
  private int m_locationStatus;
  private int m_locationIconRes;
  private boolean m_isLocationServicePaused = false;
  
  private LocationService m_locationService = null; 

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

    m_timer = new VideoTimer();
    m_locationStatus = (int)LocationService.STOPPED;
    m_locationIconRes = R.drawable.ic_menu_location;
    m_locationService = new LocationService(this);
    m_isLocationServicePaused = false;
  }
  
  public void onStatusChanged(int status)
  {
    m_locationStatus = status;

    switch (status)
    {
    case LocationService.FIRST_EVENT:
      m_locationIconRes = R.drawable.ic_menu_location_found;
      break;
    case LocationService.STOPPED:
      m_locationIconRes = R.drawable.ic_menu_location;
      break;
    case LocationService.DISABLED_BY_USER:
      m_locationIconRes = R.drawable.ic_menu_location;
      break;
    case LocationService.STARTED:
      m_locationIconRes = R.drawable.ic_menu_location_search;
      break;
    default:
      m_locationIconRes = R.drawable.ic_menu_location;
    }
    
    Log.d(TAG, "StatusChanged: " + status);
  }
  
  public void onLocationChanged(long time, double latitude, double longitude, float accuracy)
  {
  }
  
  public void onLocationNotAvailable()
  {
    Log.d(TAG, "location services is disabled or not available"); 
  }

  @Override
  protected void onPause()
  {
    if (m_locationService.isActive())
    {
      m_locationService.enterBackground();
      m_isLocationServicePaused = true;
    }

    super.onPause();
  }

  @Override
  protected void onResume()
  {
    if (m_isLocationServicePaused)
    {
      m_locationService.enterForeground();
      m_isLocationServicePaused = false;
    }
    super.onResume();    
  }

  @Override
  public boolean onCreateOptionsMenu(Menu menu)
  {
    MenuInflater inflater = getMenuInflater();
    inflater.inflate(R.menu.main, menu);
    // temprorarily disable downloader in the menu
    //menu.removeItem(R.id.download_maps);
    return true;
  }
  
  @Override 
  public boolean onPrepareOptionsMenu(Menu menu)
  {
    menu.findItem(R.id.my_position).setIcon(m_locationIconRes);
    return super.onPrepareOptionsMenu(menu);
  }

  @Override
  public boolean onOptionsItemSelected(MenuItem item)
  {
    // Handle item selection
    
    switch (item.getItemId())
    {
    case R.id.my_position:

      if (m_locationService.isActive())
        m_locationService.stopUpdate(true);
      else
        m_locationService.startUpdate(this, true);
      
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
