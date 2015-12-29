package com.mapswithme.maps;

import android.app.Application;
import android.content.SharedPreferences;
import android.content.pm.PackageManager.NameNotFoundException;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.preference.PreferenceManager;
import android.util.Log;

import java.io.File;

import com.google.gson.Gson;
import com.mapswithme.country.ActiveCountryTree;
import com.mapswithme.country.CountryItem;
import com.mapswithme.maps.background.AppBackgroundTracker;
import com.mapswithme.maps.background.Notifier;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.location.TrackRecorder;
import com.mapswithme.maps.sound.TtsPlayer;
import com.mapswithme.util.Config;
import com.mapswithme.util.Constants;
import com.mapswithme.util.ThemeSwitcher;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Yota;
import com.mapswithme.util.statistics.AlohaHelper;
import com.mapswithme.util.statistics.Statistics;
import com.parse.Parse;
import com.parse.ParseException;
import com.parse.ParseInstallation;
import com.parse.SaveCallback;

public class MwmApplication extends Application
                         implements ActiveCountryTree.ActiveCountryListener
{
  private final static String TAG = "MwmApplication";

  // Parse
  private static final String PREF_PARSE_DEVICE_TOKEN = "ParseDeviceToken";
  private static final String PREF_PARSE_INSTALLATION_ID = "ParseInstallationId";

  private static MwmApplication sSelf;
  private SharedPreferences mPrefs;
  private AppBackgroundTracker mBackgroundTracker;
  private final Gson mGson = new Gson();

  private boolean mAreCountersInitialized;
  private boolean mIsFrameworkInitialized;

  private Handler mMainLoopHandler;
  private final Object mMainQueueToken = new Object();

  public MwmApplication()
  {
    super();
    sSelf = this;
  }

  public static MwmApplication get()
  {
    return sSelf;
  }

  public static Gson gson()
  {
    return sSelf.mGson;
  }

  public static AppBackgroundTracker backgroundTracker()
  {
    return sSelf.mBackgroundTracker;
  }

  public static SharedPreferences prefs()
  {
    return sSelf.mPrefs;
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
      Statistics.INSTANCE.trackMapChanged(Statistics.EventName.MAP_DOWNLOADED);
    else if (oldGroup == ActiveCountryTree.GROUP_OUT_OF_DATE && newGroup == ActiveCountryTree.GROUP_UP_TO_DATE)
      Statistics.INSTANCE.trackMapChanged(Statistics.EventName.MAP_UPDATED);
  }

  @Override
  public void onCountryOptionsChanged(int group, int position, int newOptions, int requestOptions) {}

  @Override
  public void onCreate()
  {
    super.onCreate();
    mMainLoopHandler = new Handler(getMainLooper());

    initPaths();
    nativeInitPlatform(getApkPath(), getDataStoragePath(), getTempPath(), getObbGooglePath(),
                       BuildConfig.FLAVOR, BuildConfig.BUILD_TYPE,
                       Yota.isFirstYota(), UiUtils.isTablet());
    initParse();
    mPrefs = getSharedPreferences(getString(R.string.pref_file_name), MODE_PRIVATE);
    mBackgroundTracker = new AppBackgroundTracker();
    TrackRecorder.init();
  }

  public void initNativeCore()
  {
    if (mIsFrameworkInitialized)
      return;

    nativeInitFramework();
    ActiveCountryTree.addListener(this);
    initNativeStrings();
    BookmarkManager.getIcons(); // init BookmarkManager (automatically loads bookmarks)
    TtsPlayer.INSTANCE.init(this);
    ThemeSwitcher.restart();
    mIsFrameworkInitialized = true;
  }

  @SuppressWarnings("ResultOfMethodCallIgnored")
  private void initPaths()
  {
    new File(getDataStoragePath()).mkdirs();
    new File(getTempPath()).mkdirs();
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

  public boolean isFrameworkInitialized()
  {
    return mIsFrameworkInitialized;
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

  public static String getDataStoragePath()
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

  private static String getObbGooglePath()
  {
    final String storagePath = Environment.getExternalStorageDirectory().getAbsolutePath();
    return storagePath.concat(String.format(Constants.OBB_PATH, BuildConfig.APPLICATION_ID));
  }

  static
  {
    System.loadLibrary("mapswithme");
  }

  /**
   * Initializes native Platform with paths. Should be called before usage of any other native components.
   */
  private native void nativeInitPlatform(String apkPath, String storagePath, String tmpPath, String obbGooglePath,
                                         String flavorName, String buildType, boolean isYota, boolean isTablet);

  private native void nativeInitFramework();

  private native void nativeAddLocalization(String name, String value);

  /**
   * Check if device have at least {@code size} bytes free.
   */
  public native boolean hasFreeSpace(long size);

  private native void runNativeFunctor(final long functorPointer);

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
    if (!mAreCountersInitialized)
    {
      mAreCountersInitialized = true;
      Config.updateLaunchCounter();
      PreferenceManager.setDefaultValues(this, R.xml.prefs_misc, false);
    }
  }

  public static void onUpgrade()
  {
    Config.resetAppSessionCounters();
  }

  @SuppressWarnings("unused")
  public void runNativeFunctorOnUiThread(final long functorPointer)
  {
    Message m = Message.obtain(mMainLoopHandler, new Runnable()
    {
      @Override
      public void run()
      {
        runNativeFunctor(functorPointer);
      }
    });
    m.obj = mMainQueueToken;
    mMainLoopHandler.sendMessage(m);
  }

  public void clearFunctorsOnUiThread()
  {
    mMainLoopHandler.removeCallbacksAndMessages(mMainQueueToken);
  }
}