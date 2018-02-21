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

import com.appsflyer.AppsFlyerLib;
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
import com.mapswithme.maps.ugc.UGC;
import com.mapswithme.util.Config;
import com.mapswithme.util.Constants;
import com.mapswithme.util.Counters;
import com.mapswithme.util.CrashlyticsUtils;
import com.mapswithme.util.PermissionsUtils;
import com.mapswithme.util.SharedPropertiesUtils;
import com.mapswithme.util.StorageUtils;
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

import java.io.File;
import java.util.List;

public class MwmApplication extends Application
{
  private Logger mLogger;
  private final static String TAG = "MwmApplication";

  private static final String PW_EMPTY_APP_ID = "XXXXX";

  private static MwmApplication sSelf;
  private SharedPreferences mPrefs;
  private AppBackgroundTracker mBackgroundTracker;

  private boolean mFrameworkInitialized;
  private boolean mPlatformInitialized;
  private boolean mCrashlyticsInitialized;

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

  @NonNull
  private final AppBackgroundTracker.OnVisibleAppLaunchListener mVisibleAppLaunchListener =
      new AppBackgroundTracker.OnVisibleAppLaunchListener()
      {
        @Override
        public void onVisibleAppLaunch()
        {
          Statistics.INSTANCE.trackColdStartupInfo();
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

    initCoreIndependentSdks();

    mPrefs = getSharedPreferences(getString(R.string.pref_file_name), MODE_PRIVATE);
    mBackgroundTracker = new AppBackgroundTracker();
    mBackgroundTracker.addListener(mVisibleAppLaunchListener);
  }

  private void initCoreIndependentSdks()
  {
    initCrashlytics();
    initPushWoosh();
    initAppsFlyer();
  }

  /**
   * Initialize native core of application: platform and framework. Caller must handle returned value
   * and do nothing with native code if initialization is failed.
   *
   * @return boolean - indicator whether native initialization is successful or not.
   */
  public boolean initCore()
  {
    initNativePlatform();
    if (!mPlatformInitialized)
      return false;

    initNativeFramework();
    return mFrameworkInitialized;
  }

  private void initNativePlatform()
  {
    if (mPlatformInitialized)
      return;

    final boolean isInstallationIdFound = setInstallationIdToCrashlytics();

    initTracker();

    final String settingsPath = StorageUtils.getSettingsPath();
    mLogger.d(TAG, "onCreate(), setting path = " + settingsPath);
    final String filesPath = StorageUtils.getFilesPath();
    mLogger.d(TAG, "onCreate(), files path = " + filesPath);
    final String tempPath = StorageUtils.getTempPath();
    mLogger.d(TAG, "onCreate(), temp path = " + tempPath);

    // If platform directories are not created it means that native part of app will not be able
    // to work at all. So, we just ignore native part initialization in this case, e.g. when the
    // external storage is damaged or not available (read-only).
    if (!createPlatformDirectories(settingsPath, filesPath, tempPath))
      return;

    // First we need initialize paths and platform to have access to settings and other components.
    nativePreparePlatform(settingsPath);
    nativeInitPlatform(StorageUtils.getApkPath(), StorageUtils.getStoragePath(settingsPath),
                       filesPath, tempPath, StorageUtils.getObbGooglePath(), BuildConfig.FLAVOR,
                       BuildConfig.BUILD_TYPE, UiUtils.isTablet());

    Config.setStatisticsEnabled(SharedPropertiesUtils.isStatisticsEnabled());

    @SuppressWarnings("unused")
    Statistics s = Statistics.INSTANCE;

    if (!isInstallationIdFound)
      setInstallationIdToCrashlytics();

    mBackgroundTracker.addListener(mBackgroundListener);
    TrackRecorder.init();
    Editor.init();
    UGC.init();
    mPlatformInitialized = true;
  }

  private boolean createPlatformDirectories(@NonNull String settingsPath, @NonNull String filesPath,
                                            @NonNull String tempPath)
  {
    if (SharedPropertiesUtils.shouldEmulateBadExternalStorage())
      return false;

    return StorageUtils.createDirectory(settingsPath) &&
           StorageUtils.createDirectory(filesPath) &&
           StorageUtils.createDirectory(tempPath);
  }

  private void initNativeFramework()
  {
    if (mFrameworkInitialized)
      return;

    nativeInitFramework();

    MapManager.nativeSubscribe(mStorageCallbacks);

    initNativeStrings();
    BookmarkManager.loadBookmarks();
    TtsPlayer.INSTANCE.init(this);
    ThemeSwitcher.restart(false);
    LocationHelper.INSTANCE.initialize();
    RoutingController.get().initialize();
    TrafficManager.INSTANCE.initialize();
    mFrameworkInitialized = true;
  }

  private void initNativeStrings()
  {
    nativeAddLocalization("core_entrance", getString(R.string.core_entrance));
    nativeAddLocalization("core_exit", getString(R.string.core_exit));
    nativeAddLocalization("core_my_places", getString(R.string.core_my_places));
    nativeAddLocalization("core_my_position", getString(R.string.core_my_position));
    nativeAddLocalization("core_placepage_unknown_place", getString(R.string.core_placepage_unknown_place));
    nativeAddLocalization("wifi", getString(R.string.wifi));
  }

  public void initCrashlytics()
  {
    if (!isCrashlyticsEnabled())
      return;

    if (isCrashlyticsInitialized())
      return;

    Fabric.with(this, new Crashlytics(), new CrashlyticsNdk());

    nativeInitCrashlytics();

    mCrashlyticsInitialized = true;
  }

  public boolean isCrashlyticsInitialized()
  {
    return mCrashlyticsInitialized;
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

  public boolean arePlatformAndCoreInitialized()
  {
    return mFrameworkInitialized && mPlatformInitialized;
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

  private void initAppsFlyer()
  {
    // There is no necessary to use a conversion data listener for a while.
    // When it's needed keep in mind that the core can't be used from the mentioned listener unless
    // the AppsFlyer sdk initializes after core initialization.
    AppsFlyerLib.getInstance().init(PrivateVariables.appsFlyerKey(),
                                    null /* conversionDataListener */);
    AppsFlyerLib.getInstance().setDebugLog(BuildConfig.DEBUG);
    AppsFlyerLib.getInstance().startTracking(this);
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
  void forwardToMainThread(final long taskPointer)
  {
    Message m = Message.obtain(mMainLoopHandler, new Runnable()
    {
      @Override
      public void run()
      {
        nativeProcessTask(taskPointer);
      }
    });
    m.obj = mMainQueueToken;
    mMainLoopHandler.sendMessage(m);
  }

  private static native void nativePreparePlatform(String settingsPath);
  private native void nativeInitPlatform(String apkPath, String storagePath, String privatePath,
                                         String tmpPath, String obbGooglePath, String flavorName,
                                         String buildType, boolean isTablet);

  private static native void nativeInitFramework();
  private static native void nativeProcessTask(long taskPointer);
  private static native void nativeAddLocalization(String name, String value);

  @UiThread
  private static native void nativeInitCrashlytics();
}
