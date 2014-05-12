package com.mapswithme.maps;

import java.io.File;

import android.app.Activity;
import android.content.pm.PackageManager.NameNotFoundException;
import android.os.Build;
import android.os.Environment;
import android.util.Log;
import android.widget.Toast;

import com.mapswithme.maps.MapStorage.Index;
import com.mapswithme.maps.background.Notifier;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.guides.GuideInfo;
import com.mapswithme.maps.guides.GuidesUtils;
import com.mapswithme.maps.location.LocationService;
import com.mapswithme.util.FbUtil;
import com.mapswithme.util.Utils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.SimpleLogger;
import com.mapswithme.util.log.StubLogger;
import com.mobileapptracker.MobileAppTracker;


public class MWMApplication extends android.app.Application implements MapStorage.Listener
{
  private final static String TAG = "MWMApplication";

  private static MWMApplication mSelf;

  private LocationService m_location = null;
  private LocationState m_locationState = null;
  private MapStorage m_storage = null;

  private boolean m_isPro = false;
  private boolean m_isYota = false;

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
    final Notifier notifier = new Notifier(this);
    switch (m_storage.countryStatus(idx))
    {
    case MapStorage.ON_DISK:
      notifier.placeDownloadCompleted(idx, getMapStorage().countryName(idx));
      tryNotifyGuideAvailable(idx);
      break;

    case MapStorage.DOWNLOAD_FAILED:
      notifier.placeDownloadFailed(idx, getMapStorage().countryName(idx));
      break;
    }
  }

  private void tryNotifyGuideAvailable(Index idx)
  {
    if (Utils.hasAnyGoogleStoreInstalled())
    {
      final GuideInfo info = Framework.getGuideInfoForIndexWithApiCheck(idx);
      if (info != null && !GuidesUtils.isGuideInstalled(info.mAppId, this)
          && !Framework.wasAdvertised(info.mAppId))
      {
        final Notifier notifier = new Notifier(this);
        notifier.placeGuideAvailable(info.mAppId, info.mTitle, info.mMessage);
        Framework.setWasAdvertised(info.mAppId);
      }
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

    m_isPro = getPackageName().contains(".pro");
    m_isYota = Build.DEVICE.equals("yotaphone");

    // http://stackoverflow.com/questions/1440957/httpurlconnection-getresponsecode-returns-1-on-second-invocation
    if (Build.VERSION.SDK_INT <= Build.VERSION_CODES.ECLAIR_MR1)
      System.setProperty("http.keepAlive", "false");

    // get url for PRO version
    if (!m_isPro)
    {
      m_proVersionURL = BuildConfig.PRO_URL;
      Log.i(TAG, "Pro version url: " + m_proVersionURL);
    }

    final String extStoragePath = getDataStoragePath();
    final String extTmpPath = getTempPath();

    // Create folders if they don't exist
    new File(extStoragePath).mkdirs();
    new File(extTmpPath).mkdirs();

    // init native framework
    nativeInit(getApkPath(), extStoragePath, extTmpPath,
               getOBBGooglePath(), m_isPro, m_isYota);

    getMapStorage().subscribe(this);

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
    if (hasBookmarks())
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
    catch (final NameNotFoundException e)
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
    return m_isPro;
  }
  public boolean hasBookmarks()
  {
    return m_isPro || m_isYota;
  }

  public boolean isYota()
  {
    return m_isYota;
  }

  public String getProVersionURL()
  {
    return m_proVersionURL;
  }

  public String getDefaultProVersionURL()
  {
    return m_defaultProURL;
  }

  static
  {
    System.loadLibrary("mapswithme");
  }

  private native void nativeInit(String apkPath, String storagePath,
                                 String tmpPath, String obbGooglePath,
                                 boolean isPro, boolean isYota);

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

  public void onMwmStart(Activity activity)
  {
    FbUtil.activate(activity);
    trackInstallUpdate(activity);
  }

  private void trackInstallUpdate(Activity activity)
  {
    //{@ TRACKERS
    final boolean DEBUG = false;
    final Logger logger = DEBUG ? SimpleLogger.get("MAT") : StubLogger.get();

    if (!Utils.hasAnyGoogleStoreInstalled(activity))
    {
      logger.d("SKIPPING TRACKERS, DOES NOT HAVE GP");
      return;
    }

    final long DELTA = 60*1000;
    final File mwmDir = new File(getDataStoragePath());
    final boolean isNewUser = !mwmDir.exists() || (System.currentTimeMillis() - mwmDir.lastModified() >= DELTA);

    final String advId = getString(R.string.advertiser_id);
    final String convKey = getString(R.string.conversion_key);

    final boolean doTrack = !"FALSE".equalsIgnoreCase(advId);
    if (doTrack)
    {
      final MobileAppTracker mat = new MobileAppTracker(activity, advId, convKey);

      if (DEBUG)
      {
        mat.setDebugMode(true);
        mat.setAllowDuplicates(true);
      }

      if (isNewUser)
      {
        mat.trackInstall();
        logger.d("MAT", "TRACK INSTALL");
      }
      else
      {
        mat.trackUpdate();
        logger.d("MAT", "TRACK UPDATE");
      }
    }
    //{@ trackers
  }
}
