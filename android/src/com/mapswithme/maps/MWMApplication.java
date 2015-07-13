package com.mapswithme.maps;

import android.content.pm.PackageManager.NameNotFoundException;
import android.os.Bundle;
import android.os.Environment;
import android.preference.PreferenceManager;
import android.text.TextUtils;
import android.text.format.DateUtils;
import android.util.Log;

import com.mapswithme.country.ActiveCountryTree;
import com.mapswithme.country.CountryItem;
import com.mapswithme.maps.background.Notifier;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.util.Constants;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Yota;
import com.mapswithme.util.statistics.AlohaHelper;
import com.mapswithme.util.statistics.Statistics;
import com.parse.Parse;
import com.parse.ParseInstallation;

import java.io.File;
import java.util.Date;

import ru.mail.mrgservice.MRGSApplication;
import ru.mail.mrgservice.MRGSMap;
import ru.mail.mrgservice.MRGSServerData;
import ru.mail.mrgservice.MRGService;

public class MWMApplication extends android.app.Application implements ActiveCountryTree.ActiveCountryListener
{
  private final static String TAG = "MWMApplication";
  private static final String FOREGROUND_TIME_SETTING = "AllForegroundTime";
  private static final String LAUNCH_NUMBER_SETTING = "LaunchNumber"; // total number of app launches
  private static final String SESSION_NUMBER_SETTING = "SessionNumber"; // session = number of days, when app was launched
  private static final String LAST_SESSION_TIMESTAMP_SETTING = "LastSessionTimestamp"; // timestamp of last session
  private static final String FIRST_INSTALL_VERSION = "FirstInstallVersion";
  private static final String FIRST_INSTALL_FLAVOR = "FirstInstallFlavor";
  private static final String IS_PREINSTALL_ACTIVATED = "PreinstallActivated";

  private static MWMApplication mSelf;

  private boolean mAreStatsInitialised;

  public MWMApplication()
  {
    super();
    mSelf = this;
  }

  public static MWMApplication get()
  {
    return mSelf;
  }

  @Override
  public void onCountryProgressChanged(int group, int position, long[] sizes) {}

  @Override
  public void onCountryStatusChanged(int group, int position, int oldStatus, int newStatus)
  {
    Notifier.cancelDownloadSuggest();
    if (newStatus == MapStorage.DOWNLOAD_FAILED)
    {
      CountryItem item = ActiveCountryTree.getCountryItem(group, position);
      Notifier.placeDownloadFailed(ActiveCountryTree.getCoreIndex(group, position), item.getName());
    }
  }

  @Override
  public void onCountryGroupChanged(int oldGroup, int oldPosition, int newGroup, int newPosition)
  {
    if (!nativeGetBoolean(IS_PREINSTALL_ACTIVATED, false) && newGroup == ActiveCountryTree.GROUP_UP_TO_DATE &&
        ((oldGroup == ActiveCountryTree.GROUP_OUT_OF_DATE) || (oldGroup == ActiveCountryTree.GROUP_NEW && ActiveCountryTree.getTotalDownloadedCount() > 1)))
    {
      nativeSetBoolean(IS_PREINSTALL_ACTIVATED, true);
      Statistics.INSTANCE.trackPreinstallActivation(getFirstInstallFlavor());
    }
  }

  @Override
  public void onCountryOptionsChanged(int group, int position, int newOptions, int requestOptions)
  {
    CountryItem item = ActiveCountryTree.getCountryItem(group, position);
    if (item.getStatus() != MapStorage.ON_DISK)
      return;

    if (newOptions == requestOptions)
      Notifier.placeDownloadCompleted(ActiveCountryTree.getCoreIndex(group, position), item.getName());
  }

  @Override
  public void onCreate()
  {
    super.onCreate();

    final String extStoragePath = getDataStoragePath();
    final String extTmpPath = getTempPath();

    // Create folders if they don't exist
    new File(extStoragePath).mkdirs();
    new File(extTmpPath).mkdirs();

    // init native framework
    nativeInit(getApkPath(), extStoragePath, extTmpPath, getOBBGooglePath(),
        BuildConfig.FLAVOR, BuildConfig.BUILD_TYPE,
        Yota.isFirstYota(), UiUtils.isSmallTablet() || UiUtils.isBigTablet());

    ActiveCountryTree.addListener(this);

    // init cross-platform strings bundle
    nativeAddLocalization("country_status_added_to_queue", getString(R.string.country_status_added_to_queue));
    nativeAddLocalization("country_status_downloading", getString(R.string.country_status_downloading));
    nativeAddLocalization("country_status_download", getString(R.string.country_status_download));
    nativeAddLocalization("country_status_download_without_routing", getString(R.string.country_status_download_without_routing));
    nativeAddLocalization("country_status_download_failed", getString(R.string.country_status_download_failed));
    nativeAddLocalization("try_again", getString(R.string.try_again));
    nativeAddLocalization("not_enough_free_space_on_sdcard", getString(R.string.not_enough_free_space_on_sdcard));
    nativeAddLocalization("dropped_pin", getString(R.string.dropped_pin));
    nativeAddLocalization("my_places", getString(R.string.my_places));
    nativeAddLocalization("my_position", getString(R.string.my_position));
    nativeAddLocalization("routes", getString(R.string.routes));

    nativeAddLocalization("routing_failed_unknown_my_position", getString(R.string.routing_failed_unknown_my_position));
    nativeAddLocalization("routing_failed_has_no_routing_file", getString(R.string.routing_failed_has_no_routing_file));
    nativeAddLocalization("routing_failed_start_point_not_found", getString(R.string.routing_failed_start_point_not_found));
    nativeAddLocalization("routing_failed_dst_point_not_found", getString(R.string.routing_failed_dst_point_not_found));
    nativeAddLocalization("routing_failed_cross_mwm_building", getString(R.string.routing_failed_cross_mwm_building));
    nativeAddLocalization("routing_failed_route_not_found", getString(R.string.routing_failed_route_not_found));
    nativeAddLocalization("routing_failed_internal_error", getString(R.string.routing_failed_internal_error));

    // init BookmarkManager (automatically loads bookmarks)
    BookmarkManager.INSTANCE.getIcons();

    initParse();
  }

  private void initMrgs()
  {
    MRGService.setAppContext(this);
    final Bundle options = new Bundle();
    options.putBoolean("locations", false);
    MRGService.service(this, new MRGSServerData.MRGSServerDataDelegate()
    {
      @Override
      public void loadServerDataDidFinished(MRGSMap mrgsMap) {}

      @Override
      public void loadPromoBannersDidFinished(MRGSMap mrgsMap) {}
    }, getString(R.string.mrgs_id), getString(R.string.mrgs_key), options);

    if (getLaunchesNumber() == 1)
      MRGSApplication.instance().markAsUpdated(new Date());
  }

  public String getApkPath()
  {
    try
    {
      return getPackageManager().getApplicationInfo(BuildConfig.APPLICATION_ID, 0).sourceDir;
    } catch (final NameNotFoundException e)
    {
      Log.e(TAG, "Can't get apk path from PackageManager");
      return "";
    }
  }

  public String getDataStoragePath()
  {
    return Environment.getExternalStorageDirectory().getAbsolutePath() + Constants.MWM_DIR_POSTFIX;
  }

  public String getTempPath()
  {
    // TODO refactor
    // Can't use getExternalCacheDir() here because of API level = 7.
    return getExtAppDirectoryPath(Constants.CACHE_DIR);
  }

  public String getExtAppDirectoryPath(String folder)
  {
    final String storagePath = Environment.getExternalStorageDirectory().getAbsolutePath();
    return storagePath.concat(String.format(Constants.STORAGE_PATH, BuildConfig.APPLICATION_ID, folder));
  }

  private String getOBBGooglePath()
  {
    final String storagePath = Environment.getExternalStorageDirectory().getAbsolutePath();
    return storagePath.concat(String.format(Constants.OBB_PATH, BuildConfig.APPLICATION_ID));
  }

  // Check if we have free space on storage (writable path).
  public native boolean hasFreeSpace(long size);

  public double getForegroundTime()
  {
    return nativeGetDouble(FOREGROUND_TIME_SETTING, 0);
  }

  static
  {
    System.loadLibrary("mapswithme");
  }

  private native void nativeInit(String apkPath, String storagePath,
                                 String tmpPath, String obbGooglePath,
                                 String flavorName, String buildType,
                                 boolean isYota, boolean isTablet);

  public native boolean nativeIsBenchmarking();

  private native void nativeAddLocalization(String name, String value);

  // Dealing with Settings
  public native boolean nativeGetBoolean(String name, boolean defaultValue);

  public native void nativeSetBoolean(String name, boolean value);

  public native int nativeGetInt(String name, int defaultValue);

  public native void nativeSetInt(String name, int value);

  public native long nativeGetLong(String name, long defaultValue);

  public native void nativeSetLong(String name, long value);

  public native double nativeGetDouble(String name, double defaultValue);

  public native void nativeSetDouble(String name, double value);

  public native String nativeGetString(String name, String defaultValue);

  public native void nativeSetString(String name, String value);

  /*
   * init Parse SDK
   */
  private void initParse()
  {
    Parse.initialize(this, "***REMOVED***", "***REMOVED***");
    ParseInstallation.getCurrentInstallation().saveInBackground();

    org.alohalytics.Statistics.logEvent(AlohaHelper.PARSE_INSTALLATION_ID, ParseInstallation.getCurrentInstallation().getInstallationId());
    org.alohalytics.Statistics.logEvent(AlohaHelper.PARSE_DEVICE_TOKEN, ParseInstallation.getCurrentInstallation().getString("deviceToken"));
  }

  public void initStats()
  {
    if (!mAreStatsInitialised)
    {
      mAreStatsInitialised = true;
      updateLaunchNumbers();
      updateSessionsNumber();
      initMrgs();
      PreferenceManager.setDefaultValues(this, R.xml.preferences, false);

      org.alohalytics.Statistics.setDebugMode(BuildConfig.DEBUG);
      org.alohalytics.Statistics.setup(BuildConfig.STATISTICS_URL, this);
    }
  }

  private void updateLaunchNumbers()
  {
    final int currentLaunches = nativeGetInt(LAUNCH_NUMBER_SETTING, 0);
    if (currentLaunches == 0)
    {
      trackFirstLaunch();
      nativeSetInt(FIRST_INSTALL_VERSION, BuildConfig.VERSION_CODE);

      final String installedFlavor = getFirstInstallFlavor();
      if (TextUtils.isEmpty(installedFlavor))
        nativeSetString(FIRST_INSTALL_FLAVOR, BuildConfig.FLAVOR);
    }

    nativeSetInt(LAUNCH_NUMBER_SETTING, currentLaunches + 1);
  }

  private void updateSessionsNumber()
  {
    final int sessionNum = nativeGetInt(SESSION_NUMBER_SETTING, 0);
    final long lastSessionTimestamp = nativeGetLong(LAST_SESSION_TIMESTAMP_SETTING, 0);
    if (!DateUtils.isToday(lastSessionTimestamp))
    {
      nativeSetInt(SESSION_NUMBER_SETTING, sessionNum + 1);
      nativeSetLong(LAST_SESSION_TIMESTAMP_SETTING, System.currentTimeMillis());
    }
  }

  private void trackFirstLaunch()
  {
    Statistics.INSTANCE.trackFirstLaunch(BuildConfig.IS_PREINSTALLED, BuildConfig.FLAVOR);
  }

  /**
   * @return total number of application launches
   */
  public int getLaunchesNumber()
  {
    return nativeGetInt(LAUNCH_NUMBER_SETTING, 0);
  }

  /**
   * Session = single day, when app was started any number of times.
   *
   * @return number of sessions.
   */
  public int getSessionsNumber()
  {
    return nativeGetInt(SESSION_NUMBER_SETTING, 0);
  }

  public int getFirstInstallVersion()
  {
    return nativeGetInt(FIRST_INSTALL_VERSION, 0);
  }

  public String getFirstInstallFlavor()
  {
    return nativeGetString(FIRST_INSTALL_FLAVOR, "");
  }
}
