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
import android.util.Log;
import android.widget.Toast;

import com.mapswithme.maps.MapStorage.Index;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.location.LocationService;
import com.mapswithme.maps.state.AppStateManager;
import com.mapswithme.maps.state.SuppotedState;
import com.mapswithme.util.Utils;


public class MWMApplication extends android.app.Application implements MapStorage.Listener
{
  private final static String TAG = "MWMApplication";

  private static MWMApplication mSelf;


  private LocationService m_location = null;
  private LocationState m_locationState = null;
  private MapStorage m_storage = null;
  private int m_slotID = 0;

  private AppStateManager mAppStateManager = new AppStateManager();

  private boolean m_isProVersion = false;

  // Set default string to Google Play page.
  private final static String m_defaultProURL = "http://play.google.com/store/apps/details?id=com.mapswithme.maps.pro";
  private String m_proVersionURL = m_defaultProURL;


  public MWMApplication()
  {
    super();
    mSelf = this;
  }

  /**
   * Just for convenience.
   *
   * @return global MWMApp
   */
  public static MWMApplication get()
  {
    return mSelf;
  }

  private void showDownloadToast(int resID, Index idx)
  {
    final String msg = String.format(getString(resID), m_storage.countryName(idx));
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

    m_isProVersion = getPackageName().endsWith(".pro");

    // http://stackoverflow.com/questions/1440957/httpurlconnection-getresponsecode-returns-1-on-second-invocation
    if (Build.VERSION.SDK_INT <= Build.VERSION_CODES.ECLAIR_MR1)
      System.setProperty("http.keepAlive", "false");

    // get url for PRO version
    if (!m_isProVersion)
    {
      AssetManager assets = getAssets();
      InputStream stream = null;
      try
      {
        stream = assets.open("app_info.txt");
        BufferedReader reader = new BufferedReader(new InputStreamReader(stream));

        final String s = reader.readLine();
        if (s.length() > 0)
          m_proVersionURL = s;

        Log.i(TAG, "Pro version url: " + m_proVersionURL);
      }
      catch (IOException ex)
      {
        // suppress exceptions - pro version doesn't need app_info.txt
      }
      Utils.closeStream(stream);
    }

    final String extStoragePath = getDataStoragePath();
    final String extTmpPath = getTempPath();

    // Create folders if they don't exist
    new File(extStoragePath).mkdirs();
    new File(extTmpPath).mkdirs();

    // init native framework
    nativeInit(getApkPath(), extStoragePath, extTmpPath,
               getOBBGooglePath(), m_isProVersion);

    m_slotID = getMapStorage().subscribe(this);

    // init cross-platform strings bundle
    nativeAddLocalization("country_status_added_to_queue", getString(R.string.country_status_added_to_queue));
    nativeAddLocalization("country_status_downloading", getString(R.string.country_status_downloading));
    nativeAddLocalization("country_status_download", getString(R.string.country_status_download));
    nativeAddLocalization("country_status_download_failed", getString(R.string.country_status_download_failed));
    nativeAddLocalization("try_again", getString(R.string.try_again));
    nativeAddLocalization("not_enough_free_space_on_sdcard", getString(R.string.not_enough_free_space_on_sdcard));
    nativeAddLocalization("dropped_pin", getString(R.string.dropped_pin));
    nativeAddLocalization("my_places", getString(R.string.my_places));
    nativeAddLocalization("my_position", getString(R.string.my_position));

    // init BookmarkManager (automatically loads bookmarks)
    if (m_isProVersion)
      BookmarkManager.getBookmarkManager(getApplicationContext());
  }

  public LocationService getLocationService()
  {
    if (m_location == null)
      m_location = new LocationService(this);

    return m_location;
  }

  public LocationState getLocationState()
  {
    if (m_locationState == null)
      m_locationState = new LocationState();

    return m_locationState;
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

  public String getTempPath()
  {
    // Can't use getExternalCacheDir() here because of API level = 7.
    return getExtAppDirectoryPath("cache");
  }

  public String getExtAppDirectoryPath(String folder)
  {
    final String storagePath = Environment.getExternalStorageDirectory().getAbsolutePath();
    return storagePath.concat(String.format("/Android/data/%s/%s/", getPackageName(), folder));
  }

  private String getOBBGooglePath()
  {
    final String storagePath = Environment.getExternalStorageDirectory().getAbsolutePath();
    return storagePath.concat(String.format("/Android/obb/%s/", getPackageName()));
  }

  /// Check if we have free space on storage (writable path).
  public native boolean hasFreeSpace(long size);

  public double getForegroundTime()
  {
    return nativeGetDouble("AllForegroundTime", 0);
  }

  public boolean isProVersion()
  {
    return m_isProVersion;
  }

  public String getProVersionURL()
  {
    return m_proVersionURL;
  }

  public String getDefaultProVersionURL()
  {
    return m_defaultProURL;
  }

  public AppStateManager getAppStateManager()
  {
    return mAppStateManager;
  }

  public SuppotedState getAppState()
  {
    return mAppStateManager.getCurrentState();
  }

  static
  {
    System.loadLibrary("mapswithme");
  }

  private native void nativeInit(String apkPath, String storagePath,
                                 String tmpPath, String obbGooglePath,
                                 boolean isPro);

  public native boolean nativeIsBenchmarking();

  /// @name Dealing with dialogs.
  /// @note Constants should be equal with map/dialog_settings.hpp
  /// @{
  static public final int FACEBOOK = 0;
  static public final int BUYPRO = 1;
  public native boolean shouldShowDialog(int dlg);

  static public final int OK = 0;
  static public final int LATER = 1;
  static public final int NEVER = 2;
  public native void submitDialogResult(int dlg, int res);
  /// @}

  private native void nativeAddLocalization(String name, String value);

  /// Dealing with Settings
  public native boolean nativeGetBoolean(String name, boolean defaultValue);
  public native void nativeSetBoolean(String name, boolean value);
  public native int nativeGetInt(String name, int defaultValue);
  public native void nativeSetInt(String name, int value);
  public native long nativeGetLong(String name, long defaultValue);
  public native void nativeSetLong(String name, long value);

  public native double nativeGetDouble(String name, double defaultValue);
  public native void nativeSetDouble(String name, double value);
}
