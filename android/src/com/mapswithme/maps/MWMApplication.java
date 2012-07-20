package com.mapswithme.maps;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;

import android.content.pm.PackageManager.NameNotFoundException;
import android.content.res.AssetManager;
import android.os.Build;
import android.os.Environment;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock;
import android.util.Log;
import android.widget.Toast;

import com.mapswithme.maps.MapStorage.Index;
import com.mapswithme.maps.location.LocationService;
import com.mapswithme.util.Utils;


public class MWMApplication extends android.app.Application implements MapStorage.Listener
{
  private final static String TAG = "MWMApplication";

  private LocationService m_location = null;
  private MapStorage m_storage = null;
  private int m_slotID = 0;

  private boolean mIsProVersion = false;
  private String mProVersionCheckURL = "";


  private void showDownloadToast(int resID, Index idx)
  {
    String msg = String.format(getString(resID), m_storage.countryName(idx));
    Toast.makeText(this, msg, Toast.LENGTH_LONG).show();
  }

  @Override
  public void onCountryStatusChanged(Index idx)
  {
    switch (m_storage.countryStatus(idx))
    {
    case MapStorage.ON_DISK:
      showDownloadToast(R.string.download_country_success, idx);
      break;

    case MapStorage.DOWNLOAD_FAILED:
      showDownloadToast(R.string.download_country_failed, idx);
      break;
    }
  }

  @Override
  public void onCountryProgress(Index idx, long current, long total)
  {
  }

  @Override
  public void onCreate()
  {
    super.onCreate();

    mIsProVersion = getPackageName().endsWith(".pro");

    // http://stackoverflow.com/questions/1440957/httpurlconnection-getresponsecode-returns-1-on-second-invocation
    if (Integer.parseInt(Build.VERSION.SDK) < Build.VERSION_CODES.FROYO)
      System.setProperty("http.keepAlive", "false");

    AssetManager assets = getAssets();
    InputStream stream = null;
    try
    {
      stream = assets.open("app_info.txt");
      BufferedReader reader = new BufferedReader(new InputStreamReader(stream));
      mProVersionCheckURL = reader.readLine();

      Log.i(TAG, "PROCHECKURL: " + mProVersionCheckURL);
    }
    catch (IOException ex)
    {
      // suppress exceptions - pro version doesn't need app_info.txt
    }

    Utils.closeStream(stream);

    final String extStoragePath = getDataStoragePath();
    final String extTmpPath = getExtAppDirectoryPath("caches");
    // Create folders if they don't exist
    new File(extStoragePath).mkdirs();
    new File(extTmpPath).mkdirs();

    nativeInit(getApkPath(),
               extStoragePath,
               getTmpPath(),
               extTmpPath,
               // Changed path for settings to be the same as external storage
               extStoragePath, //getSettingsPath(),
               mIsProVersion);

    m_slotID = getMapStorage().subscribe(this);
  }

  public LocationService getLocationService()
  {
    if (m_location == null)
      m_location = new LocationService(this);

    return m_location;
  }

  public MapStorage getMapStorage()
  {
    if (m_storage == null)
      m_storage = new MapStorage();

    return m_storage;
  }


  public String getApkPath()
  {
    try
    {
      return getPackageManager().getApplicationInfo(getPackageName(), 0).sourceDir;
    }
    catch (NameNotFoundException e)
    {
      Log.e(TAG, "Can't get apk path from PackageManager");
      return "";
    }
  }

  public String getDataStoragePath()
  {
    return Environment.getExternalStorageDirectory().getAbsolutePath() + "/MapsWithMe/";
  }

  public String getExtAppDirectoryPath(String folder)
  {
    final String storagePath = Environment.getExternalStorageDirectory().getAbsolutePath();
    return storagePath.concat(String.format("/Android/data/%s/%s/", getPackageName(), folder));
  }

  public boolean isProVersion()
  {
    return mIsProVersion;
  }

  public String getProVersionCheckURL()
  {
    return mProVersionCheckURL;
  }

  private String getTmpPath()
  {
    return getCacheDir().getAbsolutePath() + "/";
  }

  private String getSettingsPath()
  {
    return getFilesDir().getAbsolutePath() + "/";
  }

  private WakeLock mWakeLock = null;

  public void disableAutomaticStandby()
  {
    if (mWakeLock == null)
    {
      PowerManager pm = (PowerManager) getSystemService(android.content.Context.POWER_SERVICE);
      mWakeLock = pm.newWakeLock(PowerManager.SCREEN_BRIGHT_WAKE_LOCK | PowerManager.ON_AFTER_RELEASE, TAG);
      mWakeLock.acquire();
    }
  }

  public void enableAutomaticStandby()
  {
    if (mWakeLock != null)
    {
      mWakeLock.release();
      mWakeLock = null;
    }
  }


  static
  {
    System.loadLibrary("mapswithme");
  }

  private native void nativeInit(String apkPath,
                                 String storagePath,
                                 String tmpPath,
                                 String extTmpPath,
                                 String settingsPath,
                                 boolean isPro);

  public native boolean nativeIsBenchmarking();
}
