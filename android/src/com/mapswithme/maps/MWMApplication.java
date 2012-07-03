package com.mapswithme.maps;

import java.io.File;

import android.content.pm.PackageManager.NameNotFoundException;
import android.os.Environment;

import com.mapswithme.maps.location.LocationService;


public class MWMApplication extends android.app.Application
{
  private LocationService mLocationService = null;

  @Override
  public void onCreate()
  {
    super.onCreate();

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
    return true;
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
