package com.mapswithme.maps;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;

import android.content.pm.PackageManager.NameNotFoundException;
import android.content.res.AssetManager;
import android.os.Environment;
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

    AssetManager assets = getAssets();

    mIsProVersion = getPackageName().endsWith(".pro");

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
               getSettingsPath());
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

  static
  {
    System.loadLibrary("mapswithme");
  }

  private native void nativeInit(String apkPath,
                                 String storagePath,
                                 String tmpPath,
                                 String extTmpPath,
                                 String settingsPath);
}
