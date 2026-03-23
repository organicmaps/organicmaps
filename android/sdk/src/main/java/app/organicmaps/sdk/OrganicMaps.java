package app.organicmaps.sdk;

import android.content.Context;
import android.content.SharedPreferences;
import androidx.annotation.IntDef;
import androidx.annotation.MainThread;
import androidx.annotation.NonNull;
import androidx.lifecycle.DefaultLifecycleObserver;
import androidx.lifecycle.LifecycleOwner;
import androidx.lifecycle.ProcessLifecycleOwner;
import app.organicmaps.sdk.bookmarks.data.BookmarkManager;
import app.organicmaps.sdk.bookmarks.data.Icon;
import app.organicmaps.sdk.downloader.Android7RootCertificateWorkaround;
import app.organicmaps.sdk.editor.OsmOAuth;
import app.organicmaps.sdk.location.LocationHelper;
import app.organicmaps.sdk.location.LocationProviderFactory;
import app.organicmaps.sdk.location.SensorHelper;
import app.organicmaps.sdk.maplayer.isolines.IsolinesManager;
import app.organicmaps.sdk.maplayer.subway.SubwayManager;
import app.organicmaps.sdk.maplayer.traffic.TrafficManager;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.search.SearchEngine;
import app.organicmaps.sdk.settings.StoragePathManager;
import app.organicmaps.sdk.sound.TtsPlayer;
import app.organicmaps.sdk.util.Config;
import app.organicmaps.sdk.util.ConnectionState;
import app.organicmaps.sdk.util.SharedPropertiesUtils;
import app.organicmaps.sdk.util.StorageUtils;
import app.organicmaps.sdk.util.log.Logger;
import app.organicmaps.sdk.util.log.LogsManager;
import java.io.IOException;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.ArrayList;
import java.util.List;

public final class OrganicMaps implements DefaultLifecycleObserver
{
  private static final String TAG = OrganicMaps.class.getSimpleName();

  private static final int NOT_STARTED = 0;
  private static final int INITIALIZING = 1;
  private static final int READY = 2;

  @Retention(RetentionPolicy.SOURCE)
  @IntDef({NOT_STARTED, INITIALIZING, READY})
  @interface CoreState
  {}

  @NonNull
  private final String mFlavor;
  @NonNull
  private final String mVersionName;

  @NonNull
  private final Context mContext;

  @NonNull
  private final SharedPreferences mPreferences;

  @NonNull
  private final IsolinesManager mIsolinesManager;
  @NonNull
  private final SubwayManager mSubwayManager;

  @NonNull
  private final LocationHelper mLocationHelper;
  @NonNull
  private final SensorHelper mSensorHelper;

  @CoreState
  private volatile int mCoreState = NOT_STARTED;
  private volatile boolean mPlatformInitialized;
  @NonNull
  private final List<Runnable> mCoreReadyCallbacks = new ArrayList<>();

  @NonNull
  public LocationHelper getLocationHelper()
  {
    return mLocationHelper;
  }

  @NonNull
  public SensorHelper getSensorHelper()
  {
    return mSensorHelper;
  }

  @NonNull
  public SubwayManager getSubwayManager()
  {
    return mSubwayManager;
  }

  @NonNull
  public IsolinesManager getIsolinesManager()
  {
    return mIsolinesManager;
  }

  @NonNull
  public String getFlavor()
  {
    return mFlavor;
  }

  @NonNull
  public String getVersionName()
  {
    return mVersionName;
  }

  public OrganicMaps(@NonNull Context context, @NonNull String flavor, @NonNull String applicationId, int versionCode,
                     @NonNull String versionName, @NonNull String fileProviderAuthority,
                     @NonNull LocationProviderFactory locationProviderFactory)
  {
    mFlavor = flavor;
    mVersionName = versionName;
    mContext = context.getApplicationContext();
    mPreferences = mContext.getSharedPreferences(context.getString(R.string.pref_file_name), Context.MODE_PRIVATE);

    // Set configuration directory as early as possible.
    // Other methods may explicitly use Config, which requires settingsDir to be set.
    final String settingsPath = StorageUtils.getSettingsPath(mContext);
    if (!StorageUtils.createDirectory(settingsPath))
      throw new AssertionError("Can't create settingsDir " + settingsPath);
    Logger.d(TAG, "Settings path = " + settingsPath);
    nativeSetSettingsDir(settingsPath);

    Config.init(mContext, mPreferences, mFlavor, applicationId, versionCode, mVersionName, fileProviderAuthority);
    OsmOAuth.init(mPreferences);
    SharedPropertiesUtils.init(mPreferences);
    LogsManager.INSTANCE.initFileLogging(mContext, mPreferences);

    Android7RootCertificateWorkaround.initializeIfNeeded(mContext);

    Icon.loadDefaultIcons(mContext.getResources(), mContext.getPackageName());

    mSensorHelper = new SensorHelper(mContext);
    mLocationHelper = new LocationHelper(mContext, mSensorHelper, locationProviderFactory);
    mIsolinesManager = new IsolinesManager();
    mSubwayManager = new SubwayManager(mContext);

    ConnectionState.INSTANCE.initialize(mContext);
  }

  /**
   * Initialize native core of application: platform and framework.
   *
   * @throws IOException - if failed to create directories. Caller must handle
   *                     the exception and do nothing with native code if initialization is failed.
   */
  public boolean init(@NonNull Runnable onComplete) throws IOException
  {
    initNativePlatform();
    return initNativeFramework(onComplete);
  }

  public boolean arePlatformAndCoreInitialized()
  {
    return mCoreState == READY && mPlatformInitialized;
  }

  public boolean isCoreInitializing()
  {
    return mCoreState == INITIALIZING;
  }

  /// Runs the action immediately if core is ready, or queues it for when core becomes ready.
  @MainThread
  public void runWhenReady(@NonNull Runnable action)
  {
    if (mCoreState == READY)
      action.run();
    else
      mCoreReadyCallbacks.add(action);
  }

  @Override
  public void onStart(@NonNull LifecycleOwner owner)
  {
    nativeOnTransit(true);
  }

  @Override
  public void onStop(@NonNull LifecycleOwner owner)
  {
    nativeOnTransit(false);
  }

  @NonNull
  public SharedPreferences getPreferences()
  {
    return mPreferences;
  }

  private void initNativePlatform() throws IOException
  {
    if (mPlatformInitialized)
      return;

    final String apkPath = StorageUtils.getApkPath(mContext);
    Logger.d(TAG, "Apk path = " + apkPath);
    // Note: StoragePathManager uses Config, which requires SettingsDir to be set.
    final String writablePath = StoragePathManager.findMapsStorage(mContext);
    Logger.d(TAG, "Writable path = " + writablePath);
    final String privatePath = StorageUtils.getPrivatePath(mContext);
    Logger.d(TAG, "Private path = " + privatePath);
    final String tempPath = StorageUtils.getTempPath(mContext);
    Logger.d(TAG, "Temp path = " + tempPath);

    // If platform directories are not created it means that native part of app will not be able
    // to work at all. So, we just ignore native part initialization in this case, e.g. when the
    // external storage is damaged or not available (read-only).
    createPlatformDirectories(writablePath, privatePath, tempPath);

    nativeInitPlatform(mContext, apkPath, writablePath, privatePath, tempPath, mFlavor, BuildConfig.BUILD_TYPE,
                       /* isTablet */ false);
    Config.setStoragePath(writablePath);
    Config.setStatisticsEnabled(SharedPropertiesUtils.isStatisticsEnabled());

    mPlatformInitialized = true;
    Logger.i(TAG, "Platform initialized");
  }

  private boolean initNativeFramework(@NonNull Runnable onComplete)
  {
    if (mCoreState == READY)
      return false;

    // Queue the external callback (whether first or subsequent caller).
    mCoreReadyCallbacks.add(onComplete);

    if (mCoreState == INITIALIZING)
      return true; // already in progress, callback queued

    mCoreState = INITIALIZING;

    // Pass a single internal callback to native. It will fire on the UI thread
    // after LoadMapsAsync completes and InitRouting() has run.
    nativeInitFramework(this::onNativeFrameworkReady);

    // These two can run now — they only need g_framework (created synchronously above).
    initNativeStrings();
    ProcessLifecycleOwner.get().getLifecycle().addObserver(this);

    return true;
  }

  /// Called from native on UI thread when LoadMapsAsync + InitRouting() completes.
  private void onNativeFrameworkReady()
  {
    SearchEngine.INSTANCE.initialize();
    BookmarkManager.loadBookmarks();
    TtsPlayer.INSTANCE.initialize(mContext);
    RoutingController.get().initialize(mLocationHelper);
    TrafficManager.INSTANCE.initialize();
    mSubwayManager.initialize();
    mIsolinesManager.initialize();

    Logger.i(TAG, "Framework initialized");

    // Drain pending callbacks into a local copy before setting READY, so that callbacks
    // which call runWhenReady() don't modify the list during iteration.
    final List<Runnable> pending = new ArrayList<>(mCoreReadyCallbacks);
    mCoreReadyCallbacks.clear();
    mCoreState = READY;

    for (Runnable cb : pending)
      cb.run();
  }

  private void createPlatformDirectories(@NonNull String writablePath, @NonNull String privatePath,
                                         @NonNull String tempPath) throws IOException
  {
    SharedPropertiesUtils.emulateBadExternalStorage(mContext);

    StorageUtils.requireDirectory(writablePath);
    StorageUtils.requireDirectory(privatePath);
    StorageUtils.requireDirectory(tempPath);
  }

  private void initNativeStrings()
  {
    nativeAddLocalization("core_entrance", mContext.getString(R.string.core_entrance));
    nativeAddLocalization("core_exit", mContext.getString(R.string.core_exit));
    nativeAddLocalization("core_my_places", mContext.getString(R.string.core_my_places));
    nativeAddLocalization("core_my_position", mContext.getString(R.string.core_my_position));
    nativeAddLocalization("core_placepage_unknown_place", mContext.getString(R.string.core_placepage_unknown_place));
    nativeAddLocalization("postal_code", mContext.getString(R.string.postal_code));
    nativeAddLocalization("wifi", mContext.getString(R.string.category_wifi));
  }

  private static native void nativeSetSettingsDir(String settingsPath);

  private static native void nativeInitPlatform(Context context, String apkPath, String writablePath,
                                                String privatePath, String tmpPath, String flavorName, String buildType,
                                                boolean isTablet);

  private static native void nativeInitFramework(@NonNull Runnable onComplete);

  private static native void nativeAddLocalization(String name, String value);

  private static native void nativeOnTransit(boolean foreground);

  static
  {
    System.loadLibrary("organicmaps");
  }
}
