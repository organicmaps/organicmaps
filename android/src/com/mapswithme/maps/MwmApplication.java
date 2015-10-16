package com.mapswithme.maps;

import android.content.SharedPreferences;
import android.content.pm.PackageManager.NameNotFoundException;
import android.os.Environment;
import android.preference.PreferenceManager;
import android.text.TextUtils;
import android.text.format.DateUtils;
import android.util.Log;

import com.google.gson.Gson;
import com.mapswithme.country.ActiveCountryTree;
import com.mapswithme.country.CountryItem;
import com.mapswithme.maps.ads.LikesManager;
import com.mapswithme.maps.background.Notifier;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.util.Constants;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Yota;
import com.mapswithme.util.statistics.AlohaHelper;
import com.mapswithme.util.statistics.Statistics;
import com.parse.Parse;
import com.parse.ParseException;
import com.parse.ParseInstallation;
import com.parse.SaveCallback;

import java.io.File;

public class MwmApplication extends android.app.Application implements ActiveCountryTree.ActiveCountryListener
{
  private final static String TAG = "MwmApplication";
  private static final String LAUNCH_NUMBER_SETTING = "LaunchNumber"; // total number of app launches
  private static final String SESSION_NUMBER_SETTING = "SessionNumber"; // session = number of days, when app was launched
  private static final String LAST_SESSION_TIMESTAMP_SETTING = "LastSessionTimestamp"; // timestamp of last session
  private static final String FIRST_INSTALL_VERSION = "FirstInstallVersion";
  private static final String FIRST_INSTALL_FLAVOR = "FirstInstallFlavor";
  // Parse
  private static final String PREF_PARSE_DEVICE_TOKEN = "ParseDeviceToken";
  private static final String PREF_PARSE_INSTALLATION_ID = "ParseInstallationId";

  private static MwmApplication mSelf;
  private final Gson mGson = new Gson();
  private static SharedPreferences mPrefs;

  private boolean mAreCountersInitialised;
  private boolean mIsFrameworkInitialized;

  public MwmApplication()
  {
    super();
    mSelf = this;
  }

  public static MwmApplication get()
  {
    return mSelf;
  }

  public static Gson gson()
  {
    return mSelf.mGson;
  }

  public static SharedPreferences prefs()
  {
    return mPrefs;
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
      Notifier.notifyDownloadFailed(ActiveCountryTree.getCoreIndex(group, position), item.getName());
    }
  }

  @Override
  public void onCountryGroupChanged(int oldGroup, int oldPosition, int newGroup, int newPosition)
  {
    if (oldGroup == ActiveCountryTree.GROUP_NEW && newGroup == ActiveCountryTree.GROUP_UP_TO_DATE)
      Statistics.INSTANCE.myTrackerTrackMapDownload();
    else if (oldGroup == ActiveCountryTree.GROUP_OUT_OF_DATE && newGroup == ActiveCountryTree.GROUP_UP_TO_DATE)
      Statistics.INSTANCE.myTrackerTrackMapUpdate();
  }

  @Override
  public void onCountryOptionsChanged(int group, int position, int newOptions, int requestOptions) {}

  @Override
  public void onCreate()
  {
    super.onCreate();

    initParse();
    mPrefs = getSharedPreferences(getString(R.string.pref_file_name), MODE_PRIVATE);
  }

  public synchronized void initNativeCore()
  {
    if (mIsFrameworkInitialized)
      return;

    initPaths();
    nativeInit(getApkPath(), getDataStoragePath(), getTempPath(), getObbGooglePath(),
               BuildConfig.FLAVOR, BuildConfig.BUILD_TYPE,
               Yota.isFirstYota(), UiUtils.isSmallTablet() || UiUtils.isBigTablet());
    ActiveCountryTree.addListener(this);
    initNativeStrings();
    BookmarkManager.getIcons(); // init BookmarkManager (automatically loads bookmarks)
    mIsFrameworkInitialized = true;
  }

  @SuppressWarnings("ResultOfMethodCallIgnored")
  private void initPaths()
  {
    final String extStoragePath = getDataStoragePath();
    final String extTmpPath = getTempPath();

    new File(extStoragePath).mkdirs();
    new File(extTmpPath).mkdirs();
  }

  private void initNativeStrings()
  {
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
    final File cacheDir = getExternalCacheDir();
    if (cacheDir != null)
      return cacheDir.getAbsolutePath();

    return Environment.getExternalStorageDirectory().getAbsolutePath() +
        String.format(Constants.STORAGE_PATH, BuildConfig.APPLICATION_ID, Constants.CACHE_DIR);
  }

  private String getObbGooglePath()
  {
    final String storagePath = Environment.getExternalStorageDirectory().getAbsolutePath();
    return storagePath.concat(String.format(Constants.OBB_PATH, BuildConfig.APPLICATION_ID));
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

  /**
   * Check if device have at least {@code size} bytes free.
   */
  public native boolean hasFreeSpace(long size);

  /*
   * init Parse SDK
   */
  private void initParse()
  {
    Parse.initialize(this, PrivateVariables.parseApplicationId(), PrivateVariables.parseClientKey());
    ParseInstallation.getCurrentInstallation().saveInBackground(new SaveCallback()
    {
      @Override
      public void done(ParseException e)
      {
        SharedPreferences prefs = prefs();
        String previousId = prefs.getString(PREF_PARSE_INSTALLATION_ID, "");
        String previousToken = prefs.getString(PREF_PARSE_DEVICE_TOKEN, "");

        String newId = ParseInstallation.getCurrentInstallation().getInstallationId();
        String newToken = ParseInstallation.getCurrentInstallation().getString("deviceToken");
        if (!previousId.equals(newId) || !previousToken.equals(newToken))
        {
          org.alohalytics.Statistics.logEvent(AlohaHelper.PARSE_INSTALLATION_ID, newId);
          org.alohalytics.Statistics.logEvent(AlohaHelper.PARSE_DEVICE_TOKEN, newToken);
          prefs.edit()
               .putString(PREF_PARSE_INSTALLATION_ID, newId)
               .putString(PREF_PARSE_DEVICE_TOKEN, newToken).apply();
        }
      }
    });
  }

  public void initCounters()
  {
    if (!mAreCountersInitialised)
    {
      mAreCountersInitialised = true;
      updateLaunchNumbers();
      updateSessionsNumber();
      PreferenceManager.setDefaultValues(this, R.xml.preferences, false);
    }
  }

  public void onUpgrade()
  {
    nativeSetInt(LAUNCH_NUMBER_SETTING, 0);
    nativeSetInt(SESSION_NUMBER_SETTING, 0);
    nativeSetLong(LAST_SESSION_TIMESTAMP_SETTING, 0);
    nativeSetInt(LikesManager.LAST_RATED_SESSION, 0);
    updateSessionsNumber();
  }

  private void updateLaunchNumbers()
  {
    final int currentLaunches = nativeGetInt(LAUNCH_NUMBER_SETTING, 0);
    if (currentLaunches == 0)
    {
      final int installedVersion = getFirstInstallVersion();
      if (installedVersion == 0)
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
