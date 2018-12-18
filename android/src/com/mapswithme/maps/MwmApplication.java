package com.mapswithme.maps;

import android.app.Application;
import android.content.Context;
import android.content.SharedPreferences;
import android.os.Handler;
import android.os.Message;
import android.support.annotation.NonNull;
import android.support.multidex.MultiDex;
import android.util.Log;

import com.appsflyer.AppsFlyerLib;
import com.mapswithme.maps.analytics.ExternalLibrariesMediator;
import com.mapswithme.maps.background.AppBackgroundTracker;
import com.mapswithme.maps.background.NotificationChannelFactory;
import com.mapswithme.maps.background.NotificationChannelProvider;
import com.mapswithme.maps.background.Notifier;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.downloader.CountryItem;
import com.mapswithme.maps.downloader.MapManager;
import com.mapswithme.maps.editor.Editor;
import com.mapswithme.maps.geofence.GeofenceRegistry;
import com.mapswithme.maps.geofence.GeofenceRegistryImpl;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.location.TrackRecorder;
import com.mapswithme.maps.maplayer.subway.SubwayManager;
import com.mapswithme.maps.maplayer.traffic.TrafficManager;
import com.mapswithme.maps.base.MediaPlayerWrapper;
import com.mapswithme.maps.routing.RoutingController;
import com.mapswithme.maps.scheduling.ConnectivityJobScheduler;
import com.mapswithme.maps.scheduling.ConnectivityListener;
import com.mapswithme.maps.sound.TtsPlayer;
import com.mapswithme.maps.ugc.UGC;
import com.mapswithme.util.Config;
import com.mapswithme.util.Counters;
import com.mapswithme.util.KeyValue;
import com.mapswithme.util.SharedPropertiesUtils;
import com.mapswithme.util.StorageUtils;
import com.mapswithme.util.ThemeSwitcher;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;
import com.mapswithme.util.statistics.Statistics;

import java.util.HashMap;
import java.util.List;

public class MwmApplication extends Application
{
  private Logger mLogger;
  private final static String TAG = "MwmApplication";

  private static MwmApplication sSelf;
  private SharedPreferences mPrefs;
  private AppBackgroundTracker mBackgroundTracker;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private SubwayManager mSubwayManager;

  private boolean mFrameworkInitialized;
  private boolean mPlatformInitialized;

  private Handler mMainLoopHandler;
  private final Object mMainQueueToken = new Object();
  @NonNull
  private final AppBackgroundTracker.OnVisibleAppLaunchListener mVisibleAppLaunchListener = new VisibleAppLaunchListener();
  @SuppressWarnings("NullableProblems")
  @NonNull
  private ConnectivityListener mConnectivityListener;
  @NonNull
  private final MapManager.StorageCallback mStorageCallbacks = new StorageCallbackImpl();
  @SuppressWarnings("NullableProblems")
  @NonNull
  private AppBackgroundTracker.OnTransitionListener mBackgroundListener;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private ExternalLibrariesMediator mMediator;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private PurchaseOperationObservable mPurchaseOperationObservable;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private MediaPlayerWrapper mPlayer;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private GeofenceRegistry mGeofenceRegistry;

  @NonNull
  public SubwayManager getSubwayManager()
  {
    return mSubwayManager;
  }

  public MwmApplication()
  {
    super();
    sSelf = this;
  }

  @Deprecated
  public static MwmApplication get()
  {
    return sSelf;
  }

  /**
   *
   * Use {@link #backgroundTracker(Context)} instead.
   */
  @Deprecated
  public static AppBackgroundTracker backgroundTracker()
  {
    return sSelf.mBackgroundTracker;
  }

  @NonNull
  public static AppBackgroundTracker backgroundTracker(@NonNull Context context)
  {
    return ((MwmApplication) context.getApplicationContext()).getBackgroundTracker();
  }

  /**
   *
   * Use {@link #prefs(Context)} instead.
   */
  @Deprecated
  public synchronized static SharedPreferences prefs()
  {
    if (sSelf.mPrefs == null)
      sSelf.mPrefs = sSelf.getSharedPreferences(sSelf.getString(R.string.pref_file_name), MODE_PRIVATE);

    return sSelf.mPrefs;
  }

  @NonNull
  public static SharedPreferences prefs(@NonNull Context context)
  {
    String prefFile = context.getString(R.string.pref_file_name);
    return context.getSharedPreferences(prefFile, MODE_PRIVATE);
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
    mBackgroundListener = new TransitionListener(this);
    LoggerFactory.INSTANCE.initialize(this);
    mLogger = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
    mLogger.d(TAG, "Application is created");
    mMainLoopHandler = new Handler(getMainLooper());
    mMediator = new ExternalLibrariesMediator(this);
    mMediator.initSensitiveDataToleranceLibraries();
    mMediator.initSensitiveDataStrictLibrariesAsync();
    Statistics.INSTANCE.setMediator(mMediator);

    mPrefs = getSharedPreferences(getString(R.string.pref_file_name), MODE_PRIVATE);
    initNotificationChannels();

    mBackgroundTracker = new AppBackgroundTracker();
    mBackgroundTracker.addListener(mVisibleAppLaunchListener);
    mSubwayManager = new SubwayManager(this);
    mConnectivityListener = new ConnectivityJobScheduler(this);
    mConnectivityListener.listen();

    mPurchaseOperationObservable = new PurchaseOperationObservable();
    mPlayer = new MediaPlayerWrapper(this);
    mGeofenceRegistry = new GeofenceRegistryImpl(this);
  }

  private void initNotificationChannels()
  {
    NotificationChannelProvider channelProvider = NotificationChannelFactory.createProvider(this);
    channelProvider.setUGCChannel();
    channelProvider.setDownloadingChannel();
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

    final boolean isInstallationIdFound = mMediator.setInstallationIdToCrashlytics();

    final String settingsPath = StorageUtils.getSettingsPath();
    mLogger.d(TAG, "onCreate(), setting path = " + settingsPath);
    final String filesPath = StorageUtils.getFilesPath(this);
    mLogger.d(TAG, "onCreate(), files path = " + filesPath);
    final String tempPath = StorageUtils.getTempPath(this);
    mLogger.d(TAG, "onCreate(), temp path = " + tempPath);

    // If platform directories are not created it means that native part of app will not be able
    // to work at all. So, we just ignore native part initialization in this case, e.g. when the
    // external storage is damaged or not available (read-only).
    if (!createPlatformDirectories(settingsPath, filesPath, tempPath))
      return;

    // First we need initialize paths and platform to have access to settings and other components.
    nativeInitPlatform(StorageUtils.getApkPath(this), StorageUtils.getStoragePath(settingsPath),
                       filesPath, tempPath, StorageUtils.getObbGooglePath(), BuildConfig.FLAVOR,
                       BuildConfig.BUILD_TYPE, UiUtils.isTablet());

    Config.setStatisticsEnabled(SharedPropertiesUtils.isStatisticsEnabled());

    @SuppressWarnings("unused")
    Statistics s = Statistics.INSTANCE;

    if (!isInstallationIdFound)
      mMediator.setInstallationIdToCrashlytics();

    mBackgroundTracker.addListener(mBackgroundListener);
    TrackRecorder.init();
    Editor.init(this);
    UGC.init(this);
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
    SubwayManager.from(this).initialize();
    mPurchaseOperationObservable.initialize();
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

  public boolean arePlatformAndCoreInitialized()
  {
    return mFrameworkInitialized && mPlatformInitialized;
  }

  @NonNull
  public AppBackgroundTracker getBackgroundTracker()
  {
    return mBackgroundTracker;
  }

  static
  {
    System.loadLibrary("mapswithme");
  }

  @SuppressWarnings("unused")
  void sendAppsFlyerTags(@NonNull String tag, @NonNull KeyValue[] params)
  {
    HashMap<String, Object> paramsMap = new HashMap<>();
    for (KeyValue p : params)
      paramsMap.put(p.mKey, p.mValue);
    AppsFlyerLib.getInstance().trackEvent(this, tag, paramsMap);
  }

  @SuppressWarnings("unused")
  void sendPushWooshTags(String tag, String[] values)
  {
    getMediator().getEventLogger().sendTags(tag, values);
  }

  @NonNull
  public ExternalLibrariesMediator getMediator()
  {
    return mMediator;
  }

  @NonNull
  PurchaseOperationObservable getPurchaseOperationObservable()
  {
    return mPurchaseOperationObservable;
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

  @NonNull
  public ConnectivityListener getConnectivityListener()
  {
    return mConnectivityListener;
  }

  @NonNull
  public MediaPlayerWrapper getMediaPlayer()
  {
    return mPlayer;
  }

  @NonNull
  public GeofenceRegistry getGeofenceRegistry()
  {
    return mGeofenceRegistry;
  }

  private native void nativeInitPlatform(String apkPath, String storagePath, String privatePath,
                                         String tmpPath, String obbGooglePath, String flavorName,
                                         String buildType, boolean isTablet);
  private static native void nativeInitFramework();
  private static native void nativeProcessTask(long taskPointer);
  private static native void nativeAddLocalization(String name, String value);

  private static class VisibleAppLaunchListener implements AppBackgroundTracker.OnVisibleAppLaunchListener
  {
    @Override
    public void onVisibleAppLaunch()
    {
      Statistics.INSTANCE.trackColdStartupInfo();
    }
  }

  private class StorageCallbackImpl implements MapManager.StorageCallback
  {
    @Override
    public void onStatusChanged(List<MapManager.StorageCallbackData> data)
    {
      Notifier notifier = Notifier.from(MwmApplication.this);
      for (MapManager.StorageCallbackData item : data)
        if (item.isLeafNode && item.newStatus == CountryItem.STATUS_FAILED)
        {
          if (MapManager.nativeIsAutoretryFailed())
          {
            notifier.notifyDownloadFailed(item.countryId, MapManager.nativeGetName(item.countryId));
            MapManager.sendErrorStat(Statistics.EventName.DOWNLOADER_ERROR, MapManager.nativeGetError(item.countryId));
          }

          return;
        }
    }

    @Override
    public void onProgress(String countryId, long localSize, long remoteSize) {}
  }

  private static class TransitionListener implements AppBackgroundTracker.OnTransitionListener
  {
    @NonNull
    private final MwmApplication mApplication;

    TransitionListener(@NonNull MwmApplication application)
    {
      mApplication = application;
    }

    @Override
    public void onTransit(boolean foreground)
    {
      if (!foreground && LoggerFactory.INSTANCE.isFileLoggingEnabled())
      {
        Log.i(TAG, "The app goes to background. All logs are going to be zipped.");
        LoggerFactory.INSTANCE.zipLogs(null);
      }
    }
  }
}
