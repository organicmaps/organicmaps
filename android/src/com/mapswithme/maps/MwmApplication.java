package com.mapswithme.maps;

import android.app.Application;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.pm.PackageManager.NameNotFoundException;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.support.annotation.NonNull;
import android.support.annotation.UiThread;
import android.support.multidex.MultiDex;
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
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.location.TrackRecorder;
import com.mapswithme.maps.routing.RoutingController;
import com.mapswithme.maps.settings.StoragePathManager;
import com.mapswithme.maps.sound.TtsPlayer;
import com.mapswithme.maps.traffic.TrafficManager;
import com.mapswithme.util.Config;
import com.mapswithme.util.Constants;
import com.mapswithme.util.Counters;
import com.mapswithme.util.ThemeSwitcher;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;
import com.mapswithme.util.statistics.PushwooshHelper;
import com.mapswithme.util.statistics.Statistics;
import com.my.tracker.MyTracker;
import com.my.tracker.MyTrackerParams;
import com.pushwoosh.PushManager;
import io.fabric.sdk.android.Fabric;

public class MwmApplication extends Application
{
  private Logger mLogger;
  private final static String TAG = "MwmApplication";

  private static final String PW_EMPTY_APP_ID = "XXXXX";

  private static MwmApplication sSelf;
  private SharedPreferences mPrefs;
  private AppBackgroundTracker mBackgroundTracker;

  private boolean mIsFrameworkInitialized;
  private boolean mIsPlatformInitialized;

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
          if (MapManager.nativeIsAutoretryFailed())
          {
            Notifier.cancelDownloadSuggest();

            Notifier.notifyDownloadFailed(item.countryId, MapManager.nativeGetName(item.countryId));
            MapManager.sendErrorStat(Statistics.EventName.DOWNLOADER_ERROR, MapManager.nativeGetError(item.countryId));
          }

          return;
        }
    }

    @Override
    public void onProgress(String countryId, long localSize, long remoteSize) {}
  };

  @NonNull
  private final AppBackgroundTracker.OnTransitionListener mBackgroundListener =
      new AppBackgroundTracker.OnTransitionListener()
      {
        @Override
        public void onTransit(boolean foreground)
        {
          if (!foreground && LoggerFactory.INSTANCE.isFileLoggingEnabled())
          {
            Log.i(TAG, "The app goes to background. All logs are going to be zipped.");
            LoggerFactory.INSTANCE.zipLogs(null);
          }
        }
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

  public synchronized static SharedPreferences prefs()
  {
    if (sSelf.mPrefs == null)
      sSelf.mPrefs = sSelf.getSharedPreferences(sSelf.getString(R.string.pref_file_name), MODE_PRIVATE);

    return sSelf.mPrefs;
  }

  public static boolean isCrashlyticsEnabled()
  {
    return !BuildConfig.FABRIC_API_KEY.startsWith("0000");
  }

  @Override
  protected void attachBaseContext(Context base)
  {
    super.attachBaseContext(base);
    MultiDex.install(this);
  }

  @SuppressWarnings("ResultOfMethodCallIgnored")
  @Override
  public void onCreate()
  {
    super.onCreate();
    mLogger = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
    mLogger.d(TAG, "Application is created");
    mMainLoopHandler = new Handler(getMainLooper());

    initCrashlytics();

    initPushWoosh();

    mPrefs = getSharedPreferences(getString(R.string.pref_file_name), MODE_PRIVATE);
    mBackgroundTracker = new AppBackgroundTracker();
  }

  public void initNativePlatform()
  {
    if (mIsPlatformInitialized)
      return;

    final boolean isInstallationIdFound = setInstallationIdToCrashlytics();

    initTracker();

    String settingsPath = getSettingsPath();
    mLogger.d(TAG, "onCreate(), setting path = " + settingsPath);
    String tempPath = getTempPath();
    mLogger.d(TAG, "onCreate(), temp path = " + tempPath);
    new File(settingsPath).mkdirs();
    new File(tempPath).mkdirs();

    // First we need initialize paths and platform to have access to settings and other components.
    nativePreparePlatform(settingsPath);
    nativeInitPlatform(getApkPath(), getStoragePath(settingsPath), getTempPath(), getObbGooglePath(),
                       BuildConfig.FLAVOR, BuildConfig.BUILD_TYPE, UiUtils.isTablet());

    Statistics s = Statistics.INSTANCE;

    if (!isInstallationIdFound)
      setInstallationIdToCrashlytics();

    mBackgroundTracker = new AppBackgroundTracker();
    mBackgroundTracker.addListener(mBackgroundListener);
    TrackRecorder.init();
    Editor.init();
    mIsPlatformInitialized = true;
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
    ThemeSwitcher.restart(false);
    LocationHelper.INSTANCE.initialize();
    RoutingController.get().initialize();
    TrafficManager.INSTANCE.initialize();
    mIsFrameworkInitialized = true;
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
    nativeAddLocalization("placepage_unknown_place", getString(R.string.placepage_unknown_place));
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
    nativeAddLocalization("place_page_booking_rating", getString(R.string.place_page_booking_rating));
  }

  private void initCrashlytics()
  {
    if (!isCrashlyticsEnabled())
      return;

    Fabric.with(this, new Crashlytics(), new CrashlyticsNdk());

    nativeInitCrashlytics();
  }

  private static boolean setInstallationIdToCrashlytics()
  {
    if (!isCrashlyticsEnabled())
      return false;

    final String installationId = Utils.getInstallationId();
    // If installation id is not found this means id was not
    // generated by alohalytics yet and it is a first run.
    if (TextUtils.isEmpty(installationId))
      return false;

    Crashlytics.setString("AlohalyticsInstallationId", installationId);
    return true;
  }

  public boolean isFrameworkInitialized()
  {
    return mIsFrameworkInitialized;
  }
  public boolean isPlatformInitialized()
  {
    return mIsPlatformInitialized;
  }

  public String getApkPath()
  {
    try
    {
      return getPackageManager().getApplicationInfo(BuildConfig.APPLICATION_ID, 0).sourceDir;
    } catch (final NameNotFoundException e)
    {
      mLogger.e(TAG, "Can't get apk path from PackageManager", e);
      return "";
    }
  }

  public static String getSettingsPath()
  {
    return Environment.getExternalStorageDirectory().getAbsolutePath() + Constants.MWM_DIR_POSTFIX;
  }

  private static String getStoragePath(String settingsPath)
  {
    String path = Config.getStoragePath();
    if (!TextUtils.isEmpty(path))
    {
      File f = new File(path);
      if (f.exists() && f.isDirectory())
        return path;

      path = new StoragePathManager().findMapsMeStorage(settingsPath);
      Config.setStoragePath(path);
      return path;
    }

    return settingsPath;
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

  private void initPushWoosh()
  {
    try
    {
      if (BuildConfig.PW_APPID.equals(PW_EMPTY_APP_ID))
        return;

      PushManager pushManager = PushManager.getInstance(this);

      pushManager.onStartup(this);
      pushManager.registerForPushNotifications();

      PushwooshHelper.get().setContext(this);
      PushwooshHelper.get().synchronize();
    }
    catch(Exception e)
    {
      mLogger.e("Pushwoosh", "Failed to init Pushwoosh", e);
    }
  }

  @SuppressWarnings("unused")
  void sendPushWooshTags(String tag, String[] values)
  {
    try
    {
      if (values.length == 1)
        PushwooshHelper.get().sendTag(tag, values[0]);
      else
        PushwooshHelper.get().sendTag(tag, values);
    }
    catch(Exception e)
    {
      mLogger.e("Pushwoosh", "Failed to send pushwoosh tags", e);
    }
  }

  private void initTracker()
  {
    MyTracker.setDebugMode(BuildConfig.DEBUG);
    MyTracker.createTracker(PrivateVariables.myTrackerKey(), this);
    final MyTrackerParams myParams = MyTracker.getTrackerParams();
    myParams.setDefaultVendorAppPackage();
    MyTracker.initTracker();
  }

  public static void onUpgrade()
  {
    Counters.resetAppSessionCounters();
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

  private static native void nativePreparePlatform(String settingsPath);
  private native void nativeInitPlatform(String apkPath, String storagePath, String tmpPath, String obbGooglePath,
                                         String flavorName, String buildType, boolean isTablet);

  private static native void nativeInitFramework();
  private static native void nativeProcessFunctor(long functorPointer);
  private static native void nativeAddLocalization(String name, String value);

  @UiThread
  private static native void nativeInitCrashlytics();
}
