package com.mapswithme.maps;

import java.io.File;

import com.mapswithme.maps.R;
import com.mapswithme.maps.location.LocationService;
import com.nvidia.devtech.NvEventQueueActivity;

import android.app.AlertDialog;
import android.content.DialogInterface;
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

public class MWMActivity extends NvEventQueueActivity implements LocationService.Listener
{
  VideoTimer m_timer;
  
  private static String TAG = "MWMActivity";
  private final static String PACKAGE_NAME = "com.mapswithme.maps";
  
  private int m_locationIconRes;
  private boolean m_locationStarted = false;
  
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

  private void checkMeasurementSystem()
  {
    int u;
    if (!hasMeasurementSystem())
    {
      /// checking system-default measurement system
      
      if (UnitLocale.getCurrent() == UnitLocale.Metric)
      {
        u = UNITS_METRIC;
        setupMeasurementSystem();
      }
      else
      {
        u = UNITS_FOOT;

        /// showing "select measurement system" dialog.
        AlertDialog alert = new AlertDialog.Builder(this).create();  
        alert.setCancelable(false); 

        alert.setMessage("Which measurement system do you prefer?");
      
        alert.setButton(AlertDialog.BUTTON_NEGATIVE, "Mi", new DialogInterface.OnClickListener(){
          public void onClick(DialogInterface dialog, int which){
            setMeasurementSystem(UNITS_FOOT);
            setupMeasurementSystem();
            dialog.dismiss();
            }
          });
      
        alert.setButton(AlertDialog.BUTTON_POSITIVE, "Km", new DialogInterface.OnClickListener(){
          public void onClick(DialogInterface dlg, int which){
            setMeasurementSystem(UNITS_METRIC);
            setupMeasurementSystem();
            dlg.dismiss();
            }
          });
      
        alert.show();
      }
      
      setMeasurementSystem(u);
    }
    else
      setupMeasurementSystem();
  }
  
  private native boolean hasMeasurementSystem();

  
  private final int UNITS_METRIC = 0;
  private final int UNITS_YARD = 1;
  private final int UNITS_FOOT = 2;

  private native int getMeasurementSystem();    
  private native void setMeasurementSystem(int u);
  private native void setupMeasurementSystem();
  
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

    checkMeasurementSystem();
    
    m_timer = new VideoTimer();
    m_locationIconRes = R.drawable.ic_menu_location;
    m_locationService = new LocationService(this);
  }
  
  
  public void onLocationStatusChanged(int newStatus)
  {
    switch (newStatus)
    {
    case LocationService.FIRST_EVENT:
      m_locationIconRes = R.drawable.ic_menu_location_found;
      break;
    case LocationService.STARTED:
      m_locationIconRes = R.drawable.ic_menu_location_search;
      break;
    default:
      m_locationIconRes = R.drawable.ic_menu_location;
    }
    nativeLocationStatusChanged(newStatus);
  }
  
  //@Override
  public void onLocationUpdated(long time, double lat, double lon, float accuracy)
  {
    nativeLocationUpdated(time, lat, lon, accuracy);
  }

  //@Override
  public void onCompassUpdated(long time, double magneticNorth, double trueNorth, float accuracy)
  {
    nativeCompassUpdated(time, magneticNorth, trueNorth, accuracy);
  }

  @Override
  protected void onPause()
  {
    if (m_locationStarted)
      m_locationService.stopUpdate(this);

    super.onPause();
  }

  @Override
  protected void onResume()
  {
    if (m_locationStarted)
      m_locationService.startUpdate(this, this);

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
    switch (item.getItemId())
    {
    case R.id.my_position:
      if (m_locationStarted)
        m_locationService.stopUpdate(this);
      else
        m_locationService.startUpdate(this, this);
      m_locationStarted = !m_locationStarted;
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
  private native void nativeLocationStatusChanged(int newStatus);
  private native void nativeLocationUpdated(long time, double lat, double lon, float accuracy);
  private native void nativeCompassUpdated(long time, double magneticNorth, double trueNorth, float accuracy);
}
