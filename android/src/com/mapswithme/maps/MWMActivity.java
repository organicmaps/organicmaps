package com.mapswithme.maps;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.Environment;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.Surface;
import android.view.View;
import android.webkit.WebView;

import com.mapswithme.maps.location.LocationService;
import com.nvidia.devtech.NvEventQueueActivity;

public class MWMActivity extends NvEventQueueActivity implements LocationService.Listener
{
  //VideoTimer m_timer;

  private static String TAG = "MWMActivity";

  private BroadcastReceiver m_externalStorageReceiver = null;
  private AlertDialog m_storageDisconnectedDialog = null;
  private boolean m_shouldStartLocationService = false;

  private static Context m_context = null;
  public static Context getCurrentContext() { return m_context; }

  public void checkShouldStartLocationService()
  {
    if (m_shouldStartLocationService)
    {
      mApplication.getLocationService().startUpdate(this);
      m_shouldStartLocationService = false;
    }
  }

  public void OnDownloadCountryClicked()
  {
    runOnUiThread(new Runnable()
    {
      @Override
      public void run()
      {
        nativeDownloadCountry();
      }
    });
  }

  @Override
  public void OnRenderingInitialized()
  {
    runOnUiThread(new Runnable()
    {
      @Override
      public void run()
      {
        checkShouldStartLocationService();
      }
    });

    runOnUiThread(new Runnable()
    {
      @Override
      public void run()
      {
        checkMeasurementSystem();
      }
    });
  }

  @Override
  public void ReportUnsupported()
  {
    runOnUiThread(new Runnable()
    {
      @Override
      public void run()
      {
        AlertDialog alert = new AlertDialog.Builder(getCurrentContext()).create();

        alert.setMessage(getString(R.string.unsupported_phone));

        alert.setCancelable(false);

        alert.setButton(AlertDialog.BUTTON_POSITIVE, getString(R.string.close),
                        new DialogInterface.OnClickListener()
        {
          @Override
          public void onClick(DialogInterface dlg, int which)
          {
            Activity a = (Activity)getCurrentContext();
            a.moveTaskToBack(true);
            dlg.dismiss();
          }
        });

        alert.show();
      }
    });

  }

  private void checkMeasurementSystem()
  {
    int u;
    if (!hasMeasurementSystem())
    {
      // Checking system-default measurement system
      if (UnitLocale.getCurrent() == UnitLocale.Metric)
      {
        u = UNITS_METRIC;
        setupMeasurementSystem();
      } else
      {
        u = UNITS_FOOT;

        // showing "select measurement system" dialog.
        AlertDialog alert = new AlertDialog.Builder(this).create();
        alert.setCancelable(false);

        alert.setMessage(getString(R.string.which_measurement_system));

        alert.setButton(AlertDialog.BUTTON_NEGATIVE, getString(R.string.miles),
                        new DialogInterface.OnClickListener()
        {
          @Override
          public void onClick(DialogInterface dialog, int which)
          {
            setMeasurementSystem(UNITS_FOOT);
            setupMeasurementSystem();
            dialog.dismiss();
          }
        });

        alert.setButton(AlertDialog.BUTTON_POSITIVE, getString(R.string.kilometres),
                        new DialogInterface.OnClickListener()
        {
          @Override
          public void onClick(DialogInterface dlg, int which)
          {
            setMeasurementSystem(UNITS_METRIC);
            setupMeasurementSystem();
            dlg.dismiss();
          }
        });

        alert.show();
      }

      setMeasurementSystem(u);
    } else
      setupMeasurementSystem();
  }

  private native boolean hasMeasurementSystem();

  private final int UNITS_METRIC = 0;
  private final int UNITS_YARD = 1;
  private final int UNITS_FOOT = 2;

  private native int getMeasurementSystem();

  private native void setMeasurementSystem(int u);

  private native void setupMeasurementSystem();

  private static final String PREFERENCES_MYPOSITION = "isMyPositionEnabled";

  public void onMyPositionClicked(View v)
  {
    v.setBackgroundResource(R.drawable.myposition_button_normal);
    final boolean isLocationActive = v.isSelected();
    if (isLocationActive)
      mApplication.getLocationService().stopUpdate(this);
    else
      mApplication.getLocationService().startUpdate(this);
    v.setSelected(!isLocationActive);
    // Store active state of My Position
    SharedPreferences.Editor prefsEdit = getSharedPreferences(mApplication.getPackageName(), MODE_PRIVATE).edit();
    prefsEdit.putBoolean(PREFERENCES_MYPOSITION, !isLocationActive);
    prefsEdit.commit();
  }

  public void onDownloadClicked(View v)
  {
    startActivity(new Intent(this, DownloadUI.class));
  }

  private MWMApplication mApplication;

  @Override
  public void onCreate(Bundle savedInstanceState)
  {
    // Use full-screen on Kindle Fire only
    if (android.os.Build.MODEL.equals("Kindle Fire"))
    {
      getWindow().addFlags(android.view.WindowManager.LayoutParams.FLAG_FULLSCREEN);
      getWindow().clearFlags(android.view.WindowManager.LayoutParams.FLAG_FORCE_NOT_FULLSCREEN);
    }

    super.onCreate(savedInstanceState);

    m_context = this;

    mApplication = (MWMApplication)getApplication();

    // Get screen density
    DisplayMetrics metrics = new DisplayMetrics();
    getWindowManager().getDefaultDisplay().getMetrics(metrics);

    //m_timer = new VideoTimer();
  }

  // From Location interface
  @Override
  public void onLocationStatusChanged(int newStatus)
  {
    Log.d("LOCATION", "status: " + newStatus);
    if (newStatus == LocationService.FIRST_EVENT)
      findViewById(R.id.map_button_myposition).setBackgroundResource(R.drawable.myposition_button_found);
    nativeLocationStatusChanged(newStatus);
  }

  // From Location interface
  @Override
  public void onLocationUpdated(long time, double lat, double lon, float accuracy)
  {
    nativeLocationUpdated(time, lat, lon, accuracy);
  }

  private double normalizeAngle(double a)
  {
    // normalize magneticNorth into [0, 2PI]
    if (a < 0.0)
      a += (2.0*Math.PI);
    a = a % (2.0*Math.PI);
    return a;
  }

  // From Location interface
  @Override
  public void onCompassUpdated(long time, double magneticNorth, double trueNorth, double accuracy)
  {
    // correct direction
    final int screenRotation = getWindowManager().getDefaultDisplay().getOrientation();

    double correction = 0;

    // correct due to orientation
    switch (screenRotation)
    {
    case Surface.ROTATION_90:
      correction = Math.PI / 2.0;
      break;
    case Surface.ROTATION_180:
      correction = Math.PI;
      break;
    case Surface.ROTATION_270:
      correction = (3.0 * Math.PI / 2.0);
      break;
    }

    magneticNorth += correction;
    trueNorth += correction;

    magneticNorth = normalizeAngle(magneticNorth);
    trueNorth = normalizeAngle(trueNorth);

    nativeCompassUpdated(time, magneticNorth, trueNorth, accuracy);
  }

  @Override
  protected void onStart()
  {
    super.onStart();
    // Restore My Position state on startup/activity recreation
    SharedPreferences prefs = getSharedPreferences(mApplication.getPackageName(), MODE_PRIVATE);
    final boolean isMyPositionEnabled = prefs.getBoolean(PREFERENCES_MYPOSITION, false);
    findViewById(R.id.map_button_myposition).setSelected(isMyPositionEnabled);

    nativeSetString("country_status_added_to_queue", getString(R.string.country_status_added_to_queue));
    nativeSetString("country_status_downloading", getString(R.string.country_status_downloading));
    nativeSetString("country_status_download", getString(R.string.country_status_download));
    nativeSetString("country_status_download_failed", getString(R.string.country_status_download_failed));
    nativeSetString("try_again", getString(R.string.try_again));

    nativeConnectDownloadButton();
  }

  @Override
  protected void onPause()
  {
    //< stop update only if it's started in OnRenderingInitialized
    if (!m_shouldStartLocationService)
      if (findViewById(R.id.map_button_myposition).isSelected())
        mApplication.getLocationService().stopUpdate(this);

    stopWatchingExternalStorage();

    super.onPause();
  }

  @Override
  protected void onResume()
  {
    View button = findViewById(R.id.map_button_myposition);
    if (button.isSelected())
    {
      // Change button appearance to "looking for position"
      button.setBackgroundResource(R.drawable.myposition_button_normal);
      /// and remember to start locationService updates in OnRenderingInitialized
      m_shouldStartLocationService = true;
    }

    startWatchingExternalStorage();

    super.onResume();
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
    switch (item.getItemId())
    {
    case R.id.menuitem_about_dialog:
      onAboutDialogClicked();
      return true;

    default:
      return super.onOptionsItemSelected(item);
    }
  }

  private void onAboutDialogClicked()
  {
    LayoutInflater inflater = LayoutInflater.from(this);
    View alertDialogView = inflater.inflate(R.layout.about, null);
    WebView myWebView = (WebView) alertDialogView.findViewById(R.id.webview_about);
    myWebView.loadUrl("file:///android_asset/about.html");
    AlertDialog.Builder builder = new AlertDialog.Builder(this);
    builder.setView(alertDialogView);
    builder.setTitle(R.string.about);

    builder.setPositiveButton(R.string.close, new DialogInterface.OnClickListener() {
      @Override
      public void onClick(DialogInterface dialog, int which) {
        dialog.cancel();
      }
    }).show();
  }

  // Initialized to invalid combination to force update on the first check
  private boolean m_storageAvailable = false;
  private boolean m_storageWriteable = true;

  private void updateExternalStorageState()
  {
    boolean available, writeable;
    final String state = Environment.getExternalStorageState();
    if (Environment.MEDIA_MOUNTED.equals(state))
    {
      available = writeable = true;
    } else if (Environment.MEDIA_MOUNTED_READ_ONLY.equals(state))
    {
      available = true;
      writeable = false;
    } else
      available = writeable = false;

    if (m_storageAvailable != available || m_storageWriteable != writeable)
    {
      m_storageAvailable = available;
      m_storageWriteable = writeable;
      handleExternalStorageState(available, writeable);
    }
  }

  private void handleExternalStorageState(boolean available, boolean writeable)
  {
    if (available && writeable)
    {
      // Add local maps to the model
      nativeStorageConnected();
      // enable downloader button and dismiss blocking popup
      findViewById(R.id.map_button_download).setVisibility(View.VISIBLE);
      if (m_storageDisconnectedDialog != null)
        m_storageDisconnectedDialog.dismiss();
    }
    else if (available)
    {
      // Add local maps to the model
      nativeStorageConnected();
      // disable downloader button and dismiss blocking popup
      findViewById(R.id.map_button_download).setVisibility(View.INVISIBLE);
      if (m_storageDisconnectedDialog != null)
        m_storageDisconnectedDialog.dismiss();
    }
    else
    {
      // Remove local maps from the model
      nativeStorageDisconnected();
      // enable downloader button and show blocking popup
      findViewById(R.id.map_button_download).setVisibility(View.VISIBLE);
      if (m_storageDisconnectedDialog == null)
      {
        m_storageDisconnectedDialog = new AlertDialog.Builder(this).create();
        m_storageDisconnectedDialog.setTitle(R.string.external_storage_is_not_available);
        m_storageDisconnectedDialog.setMessage(getString(R.string.disconnect_usb_cable));
        m_storageDisconnectedDialog.setCancelable(false);
      }
      m_storageDisconnectedDialog.show();
    }
  }

  private void startWatchingExternalStorage()
  {
    m_externalStorageReceiver = new BroadcastReceiver()
    {
      @Override
      public void onReceive(Context context, Intent intent)
      {
        updateExternalStorageState();
      }
    };
    IntentFilter filter = new IntentFilter();
    filter.addAction(Intent.ACTION_MEDIA_MOUNTED);
    filter.addAction(Intent.ACTION_MEDIA_REMOVED);
    filter.addAction(Intent.ACTION_MEDIA_EJECT);
    filter.addAction(Intent.ACTION_MEDIA_SHARED);
    filter.addAction(Intent.ACTION_MEDIA_UNMOUNTED);
    filter.addAction(Intent.ACTION_MEDIA_BAD_REMOVAL);
    filter.addAction(Intent.ACTION_MEDIA_UNMOUNTABLE);
    filter.addAction(Intent.ACTION_MEDIA_CHECKING);
    filter.addAction(Intent.ACTION_MEDIA_NOFS);
    filter.addDataScheme("file");
    registerReceiver(m_externalStorageReceiver, filter);
    updateExternalStorageState();
  }

  private void stopWatchingExternalStorage()
  {
    if (m_externalStorageReceiver != null)
    {
      unregisterReceiver(m_externalStorageReceiver);
      m_externalStorageReceiver = null;
    }
  }

  private native void nativeSetString(String name, String value);

  private native void nativeStorageConnected();
  private native void nativeStorageDisconnected();

  private native void nativeConnectDownloadButton();
  private native void nativeDownloadCountry();

  private native void nativeDestroy();
  private native void nativeLocationStatusChanged(int newStatus);
  private native void nativeLocationUpdated(long time, double lat, double lon, float accuracy);
  private native void nativeCompassUpdated(long time, double magneticNorth, double trueNorth, double accuracy);
}
