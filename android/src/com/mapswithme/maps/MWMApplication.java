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

import com.mapswithme.maps.location.LocationService;

public class MWMApplication extends android.app.Application
{
  private final static String TAG = "MWMApplication";
  private LocationService mLocationService = null;

  private boolean mIsProVersion = false;
  private String mProVersionCheckURL = "";

  @Override
  public void onCreate()
  {
    super.onCreate();

    mIsProVersion = getPackageName().endsWith(".pro");

    // http://stackoverflow.com/questions/1440957/httpurlconnection-getresponsecode-returns-1-on-second-invocation
    if (Integer.parseInt(Build.VERSION.SDK) < Build.VERSION_CODES.FROYO)
      System.setProperty("http.keepAlive", "false");

    AssetManager assets = getAssets();
    try
    {
      InputStream stream = assets.open("app_info.txt");
      BufferedReader reader = new BufferedReader(new InputStreamReader(stream));
      mProVersionCheckURL = reader.readLine();
      Log.i(TAG, "PROCHECKURL: " + mProVersionCheckURL);
    }
    catch (IOException e)
    {
      // TODO Auto-generated catch block
      e.printStackTrace();
    }

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
  }

  public LocationService getLocationService()
  {
    if (mLocationService == null)
      mLocationService = new LocationService(this);

    return mLocationService;
  }

  public String getApkPath()
  {
    try
    {
      return getPackageManager().getApplicationInfo(getPackageName(), 0).sourceDir;
    }
    catch (NameNotFoundException e)
    {
      e.printStackTrace();
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
