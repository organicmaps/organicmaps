package com.mapswithme.maps;

import java.io.File;

import android.app.Application;
import android.content.pm.PackageManager.NameNotFoundException;
import android.os.Environment;

public class MWMApplication extends Application
{
  public final static String PACKAGE_NAME = "com.mapswithme.maps";
  
  public void onCreate()
  {
    final String extStoragePath = getDataStoragePath();
    final String extTmpPath = getExtAppDirectoryPath("caches");
    // Create folders if they don't exist
    new File(extStoragePath).mkdirs();
    new File(extTmpPath).mkdirs();

    nativeInit(getApkPath(),
               extStoragePath, 
               getTmpPath(),
               extTmpPath,
               getSettingsPath(),
               getString(R.string.empty_model));
        
    super.onCreate();    
  }

  public String getApkPath()
  {
    try
    {
      return getPackageManager().getApplicationInfo(PACKAGE_NAME, 0).sourceDir;
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
    return storagePath.concat(String.format("/Android/data/%s/%s/", PACKAGE_NAME, folder));
  }

  private String getTmpPath()
  {
    return getCacheDir().getAbsolutePath() + "/";
  }

  private String getSettingsPath()
  {
    return getFilesDir().getAbsolutePath() + "/";
  }  
  
  public String getPackageName()
  {
    return PACKAGE_NAME;
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
                                 String emptyModelMessage);
}
