package com.mapswithme.maps;

import android.app.Application;
import android.content.SharedPreferences;
import android.content.pm.PackageManager.NameNotFoundException;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.preference.PreferenceManager;
import android.support.annotation.UiThread;
import android.text.TextUtils;
import android.util.Log;

import java.io.File;
import java.util.List;

import com.crashlytics.android.Crashlytics;
import com.crashlytics.android.ndk.CrashlyticsNdk;
import com.mapswithme.maps.background.AppBackgroundTracker;
import com.mapswithme.maps.background.Notifier;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.downloader.CountryItem;
import com.mapswithme.maps.downloader.MapManager;
import com.mapswithme.maps.editor.Editor;
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
import io.fabric.sdk.android.Fabric;
import net.hockeyapp.android.CrashManager;

public class MwmApplication extends Application
{
  private final static String TAG = "MwmApplication";

  // Parse
  private static final String PREF_PARSE_DEVICE_TOKEN = "ParseDeviceToken";
  private static final String PREF_PARSE_INSTALLATION_ID = "ParseInstallationId";

  private static MwmApplication sSelf;
  private SharedPreferences mPrefs;
  private AppBackgroundTracker mBackgroundTracker;

  private boolean mAreCountersInitialized;
  private boolean mIsFrameworkInitialized;

  private Handler mMainLoopHandler;
  private final Object mMainQueueToken = new Object();

  private final MapManager.StorageCallback mStorageCallbacks = new MapManager.StorageCallback()
  {
    @Override
    public void onStatusChanged(List<MapManager.StorageCallbackData> data)
    {
      for (MapManager.StorageCallbackData item : data)
        if (item.isLeafNode && item.newStatus == CountryItem.STATUS_FAILED)
        {
          Notifier.cancelDownloadSuggest();

          CountryItem country = CountryItem.fill(item.countryId);
          Notifier.notifyDownloadFailed(country);
          MapManager.sendErrorStat(Statistics.EventName.DOWNLOADER_ERROR, country.errorCode);

          return;
        }
    }

    @Override
    public void onProgress(String countryId, long localSize, long remoteSize) {}
  };

  public MwmApplication()
  {
    super();
    sSelf = this;
  }

  public static MwmApplication get()
  {
    return sSelf;
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
  public void onCreate()
  {
    super.onCreate();
    mMainLoopHandler = new Handler(getMainLooper());

    initHockeyApp();
    initCrashlytics();

    initPaths();
    nativeInitPlatform(getApkPath(), getDataStoragePath(), getTempPath(), getObbGooglePath(),
                       BuildConfig.FLAVOR, BuildConfig.BUILD_TYPE,
                       Yota.isFirstYota(), UiUtils.isTablet());
    initParse();
    mPrefs = getSharedPreferences(getString(R.string.pref_file_name), MODE_PRIVATE);
    mBackgroundTracker = new AppBackgroundTracker();
    TrackRecorder.init();
    Editor.init();
  }

  public void initNativeCore()
  {
    if (mIsFrameworkInitialized)
      return;

    nativeInitFramework();

    MapManager.nativeSubscribe(mStorageCallbacks);

    initNativeStrings();
    BookmarkManager.nativeLoadBookmarks();
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
    nativeAddLocalization("cancel", getString(R.string.cancel));
    nativeAddLocalization("wifi", getString(R.string.wifi));

    nativeAddLocalization("routing_failed_unknown_my_position", getString(R.string.routing_failed_unknown_my_position));
    nativeAddLocalization("routing_failed_has_no_routing_file", getString(R.string.routing_failed_has_no_routing_file));
    nativeAddLocalization("routing_failed_start_point_not_found", getString(R.string.routing_failed_start_point_not_found));
    nativeAddLocalization("routing_failed_dst_point_not_found", getString(R.string.routing_failed_dst_point_not_found));
    nativeAddLocalization("routing_failed_cross_mwm_building", getString(R.string.routing_failed_cross_mwm_building));
    nativeAddLocalization("routing_failed_route_not_found", getString(R.string.routing_failed_route_not_found));
    nativeAddLocalization("routing_failed_internal_error", getString(R.string.routing_failed_internal_error));
  }

  private void initHockeyApp()
  {
    String id = ("beta".equals(BuildConfig.BUILD_TYPE) ? PrivateVariables.hockeyAppBetaId()
                                                       : PrivateVariables.hockeyAppId());
    if (!TextUtils.isEmpty(id))
      CrashManager.register(this, id);
  }

  private void initCrashlytics()
  {
    if (BuildConfig.FABRIC_API_KEY.startsWith("0000"))
      return;

    Fabric.with(this, new Crashlytics(), new CrashlyticsNdk());
    nativeInitCrashlytics();
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

  /*
   * init Parse SDK
   */
  private void initParse()
  {
    // Do not initialize Parse in default open-source version.
    final String appId = PrivateVariables.parseApplicationId();
    if (appId.isEmpty())
      return;

    Parse.initialize(this, appId, PrivateVariables.parseClientKey());
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
  void forwardToMainThread(final long functorPointer)
  {
    Message m = Message.obtain(mMainLoopHandler, new Runnable()
    {
      @Override
      public void run()
      {
        nativeProcessFunctor(functorPointer);
      }
    });
    m.obj = mMainQueueToken;
    mMainLoopHandler.sendMessage(m);
  }

  void clearFunctorsOnUiThread()
  {
    mMainLoopHandler.removeCallbacksAndMessages(mMainQueueToken);
  }

  /**
   * Initializes native Platform with paths. Should be called before usage of any other native components.
   */
  private native void nativeInitPlatform(String apkPath, String storagePath, String tmpPath, String obbGooglePath,
                                         String flavorName, String buildType, boolean isYota, boolean isTablet);

  private static native void nativeInitFramework();

  private static native void nativeAddLocalization(String name, String value);

  private static native void nativeProcessFunctor(long functorPointer);

  @UiThread
  private static native void nativeInitCrashlytics();
}