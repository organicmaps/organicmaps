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
import android.graphics.BitmapFactory;
import android.os.Bundle;
import android.os.Environment;
import android.view.Gravity;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.widget.FrameLayout;
import android.widget.ImageButton;
import android.widget.LinearLayout;
import android.util.DisplayMetrics;
import android.util.Log;

public class MWMActivity extends NvEventQueueActivity implements LocationService.Listener
{
  VideoTimer m_timer;
  
  private static String TAG = "MWMActivity";
  private final static String PACKAGE_NAME = "com.mapswithme.maps";
  
  private int m_ic_location;
  private int m_ic_menu_location;
  private int m_ic_download;
  private int m_ic_menu_download;
  private boolean m_locationStarted = false;
  
  private BitmapFactory m_bitmapFactory = null;
  private ImageButton m_btnLocation = null;
  private ImageButton m_btnDownloadMaps = null;
  
  private LocationService m_locationService = null; 

  private String getAppBundlePath() throws NameNotFoundException
  {
    PackageManager packMgmr = getApplication().getPackageManager();
    ApplicationInfo appInfo = packMgmr.getApplicationInfo(PACKAGE_NAME, 0);
    return appInfo.sourceDir;
  }

  private String getDataStoragePath()
  {
    String storagePath = Environment.getExternalStorageDirectory()
        .getAbsolutePath();
    return storagePath.concat(String.format("/Android/data/%s/files/", PACKAGE_NAME));
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
  
  private void updateButtonIcons()
  {
    m_btnLocation.setImageBitmap(BitmapFactory.decodeResource(getResources(), m_ic_location));
    m_btnDownloadMaps.setImageBitmap(BitmapFactory.decodeResource(getResources(), m_ic_download));
  }
  
  private void onLocationClicked()
  {
    if (m_locationStarted)
      m_locationService.stopUpdate(this);
    else
      m_locationService.startUpdate(this, this);
    m_locationStarted = !m_locationStarted;
  }
  
  private void onDownloadMapsClicked()
  {
    m_ic_download = R.drawable.ic_download_highlighted;
    updateButtonIcons();
    Intent intent = new Intent(this, DownloadUI.class);
    startActivity(intent);
  }
  
  public void setupLayout()
  {
    m_bitmapFactory = new BitmapFactory();
    
    FrameLayout mapLayout = new FrameLayout(this);
    LinearLayout mapButtons = new LinearLayout(this);
    
    m_btnLocation = new ImageButton(this);

    m_btnLocation.setBackgroundDrawable(null);
    m_btnLocation.setOnClickListener(new View.OnClickListener(){
          public void onClick(View view){
            onLocationClicked();
          }});

    m_ic_location = R.drawable.ic_location;
    m_ic_menu_location = R.drawable.ic_menu_location;
    
    m_btnDownloadMaps = new ImageButton(this);
    m_btnDownloadMaps.setImageBitmap(BitmapFactory.decodeResource(getResources(), R.drawable.ic_download));
    m_btnDownloadMaps.setBackgroundDrawable(null);
    m_btnDownloadMaps.setOnClickListener(new View.OnClickListener(){
          public void onClick(View v){
            onDownloadMapsClicked();
        }});
    
    m_ic_download = R.drawable.ic_download;
    m_ic_menu_download = R.drawable.ic_menu_download;
  
    DisplayMetrics m = new DisplayMetrics();
    getWindowManager().getDefaultDisplay().getMetrics(m);
    
    Log.d(TAG, "screen density koeff is: " + m.density);
        
    LinearLayout.LayoutParams p = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.WRAP_CONTENT, 
                                                                LinearLayout.LayoutParams.WRAP_CONTENT);
    
    p.setMargins((int)(10 * m.density), 
                 (int)(10 * m.density), 
                 (int)(0 * m.density), 
                 (int)(5 * m.density));
    

    mapButtons.setGravity(Gravity.BOTTOM);    
    
    mapButtons.addView(m_btnLocation, p);

    LinearLayout.LayoutParams p1 = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.WRAP_CONTENT, 
                                                                LinearLayout.LayoutParams.WRAP_CONTENT);
    
    p1.setMargins((int)(0 * m.density), 
                 (int)(10 * m.density), 
                 (int)(0 * m.density), 
                 (int)(5 * m.density));
    
    
    mapButtons.addView(m_btnDownloadMaps, p1);

    mapLayout.addView(view3d);
    mapLayout.addView(mapButtons);

    updateButtonIcons();
    
    setContentView(mapLayout);     
  }
  
  private void setupLanguages()
  {
    /*Log.d(TAG, "Default Language : " + Locale.getDefault().getLanguage());
    for (Locale l : Locale.getAvailableLocales())
      Log.d(TAG, l.getLanguage() + " : " + l.getVariant() + " : " + l.toString());*/
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

    setupLayout();        

    setupLanguages();

    checkMeasurementSystem();

    m_timer = new VideoTimer();

    m_locationService = new LocationService(this);
  }
  
  
  public void onLocationStatusChanged(int newStatus)
  {
    switch (newStatus)
    {
    case LocationService.FIRST_EVENT:
      m_ic_location = R.drawable.ic_location_found;
      m_ic_menu_location = R.drawable.ic_menu_location_found;
      break;
    case LocationService.STARTED:
      m_ic_location = R.drawable.ic_location_search;
      m_ic_menu_location = R.drawable.ic_menu_location_search;
      break;
    default:
      m_ic_location = R.drawable.ic_location;
      m_ic_menu_location = R.drawable.ic_menu_location;
    }
    
    updateButtonIcons();
    
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
    int ic_location_old = m_ic_location;
    int ic_menu_location_old = m_ic_menu_location;
    
    if (m_locationStarted)
      m_locationService.stopUpdate(this);
    
    m_ic_location = ic_location_old;
    m_ic_menu_location = ic_menu_location_old;
    updateButtonIcons();

    super.onPause();
  }

  @Override
  protected void onResume()
  {
    m_ic_download = R.drawable.ic_download;
    m_ic_menu_download = R.drawable.ic_menu_download;
    
    updateButtonIcons();
    
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
    menu.findItem(R.id.my_position).setIcon(m_ic_menu_location);
    return super.onPrepareOptionsMenu(menu);
  }

  @Override
  public boolean onOptionsItemSelected(MenuItem item)
  {
    switch (item.getItemId())
    {
    case R.id.my_position:
      onLocationClicked();
      return true;

    case R.id.download_maps:
      onDownloadMapsClicked();
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
