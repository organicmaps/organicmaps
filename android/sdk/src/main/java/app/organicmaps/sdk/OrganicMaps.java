package app.organicmaps.sdk;

import android.content.Context;
import android.content.SharedPreferences;
import androidx.annotation.NonNull;
import androidx.lifecycle.DefaultLifecycleObserver;
import androidx.lifecycle.LifecycleOwner;
import androidx.lifecycle.ProcessLifecycleOwner;
import app.organicmaps.sdk.bookmarks.data.BookmarkManager;
import app.organicmaps.sdk.bookmarks.data.Icon;
import app.organicmaps.sdk.downloader.Android7RootCertificateWorkaround;
import app.organicmaps.sdk.editor.OsmOAuth;
import app.organicmaps.sdk.location.LocationHelper;
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

/**
 * Process-wide entry point for the native map framework.
 * Create one instance in {@link android.app.Application#onCreate} and call {@link #init} on the main thread.
 * Wait for {@link #getMapsReadiness()} before inflating a {@link MapView} or using search, editing or routing.
 * The instance retains the application context.
 * The native framework and its feature managers are shared by every map view in the process.
 */
public final class OrganicMaps implements DefaultLifecycleObserver
{
  private static final String TAG = OrganicMaps.class.getSimpleName();

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

  @NonNull
  private final MapsReadiness mMapsReadiness;

  private volatile boolean mFrameworkInitialized;
  private volatile boolean mPlatformInitialized;

  /**
   * Returns the process-wide location service. The host requests runtime permissions and manages foreground use.
   *
   * @return location service shared with the renderer and routing controller
   */
  @NonNull
  public LocationHelper getLocationHelper()
  {
    return mLocationHelper;
  }

  /**
   * Returns the compass and motion sensor service shared with the location service.
   */
  @NonNull
  public SensorHelper getSensorHelper()
  {
    return mSensorHelper;
  }

  /**
   * Returns the transit-layer manager. Use it after {@link #init} has initialized the framework.
   */
  @NonNull
  public SubwayManager getSubwayManager()
  {
    return mSubwayManager;
  }

  /**
   * Returns the contour-line layer manager. Use it after framework initialization.
   */
  @NonNull
  public IsolinesManager getIsolinesManager()
  {
    return mIsolinesManager;
  }

  /**
   * Returns the process-wide readiness of maps loaded by {@link #init}.
   */
  @NonNull
  public MapsReadiness getMapsReadiness()
  {
    return mMapsReadiness;
  }

  @NonNull
  public String getFlavor()
  {
    return mFlavor;
  }

  /**
   * Returns the host application version name supplied to the constructor, rather than the SDK artifact version.
   */
  @NonNull
  public String getVersionName()
  {
    return mVersionName;
  }

  /**
   * Creates the SDK services and configures their application-private storage.
   * Class initialization loads the native library; this constructor initializes settings and defers map loading
   * until {@link #init}.
   *
   * @param context application or activity context; only its application context is retained
   * @param flavor host-defined distribution name used by SDK configuration
   * @param applicationId host application ID
   * @param versionCode host application version code
   * @param versionName host application version name
   * @throws AssertionError if the application-private settings directory cannot be created
   */
  public OrganicMaps(@NonNull Context context, @NonNull String flavor, @NonNull String applicationId, int versionCode,
                     @NonNull String versionName)
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

    Config.init(mPreferences, mFlavor, applicationId, versionCode, mVersionName);
    OsmOAuth.init(mPreferences);
    SharedPropertiesUtils.init(mPreferences);
    LogsManager.INSTANCE.initFileLogging(mContext, mPreferences);

    Android7RootCertificateWorkaround.initializeIfNeeded(mContext);

    Icon.loadDefaultIcons(mContext.getResources(), mContext.getPackageName());

    mSensorHelper = new SensorHelper(mContext);
    mLocationHelper = new LocationHelper(mContext, mSensorHelper);
    mIsolinesManager = new IsolinesManager();
    mSubwayManager = new SubwayManager(mContext);
    mMapsReadiness = new MapsReadiness();

    ConnectionState.INSTANCE.initialize(mContext);
  }

  /**
   * Initializes the platform and framework and starts asynchronous map loading on the main thread.
   * Repeated calls share the same load. Use {@link #getMapsReadiness()} to wait for completion.
   *
   * @throws IOException if a required platform directory cannot be created
   */
  public void init() throws IOException
  {
    initNativePlatform();
    initNativeFramework();
  }

  /**
   * Reports whether platform and framework setup have run.
   * This becomes {@code true} before asynchronous map loading finishes. Use {@link MapsReadiness} to decide when
   * an activity may display its map.
   *
   * @return whether the platform and framework have been initialized
   */
  public boolean arePlatformAndCoreInitialized()
  {
    return mFrameworkInitialized && mPlatformInitialized;
  }

  /**
   * Forwards process foreground entry to the native framework. Registered automatically by {@link #init}.
   */
  @Override
  public void onStart(@NonNull LifecycleOwner owner)
  {
    nativeOnEnterForeground();
  }

  /**
   * Forwards process background entry to the native framework. Registered automatically by {@link #init}.
   */
  @Override
  public void onStop(@NonNull LifecycleOwner owner)
  {
    // UsageStats and viewport are saved earlier from MwmApplication.onActivityStopped
    // to beat task-kill. Here only heavy resource release behind PL's debounce.
    Framework.nativeReleaseBackgroundResources();
  }

  /**
   * Returns the SDK settings stored in the host application. Prefer feature APIs over editing their keys directly.
   */
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

  private void initNativeFramework()
  {
    if (mFrameworkInitialized)
      return;

    nativeInitFramework(() -> {
      RoutingController.get().initialize(mLocationHelper);
      mMapsReadiness.complete();
    });

    initNativeStrings();
    SearchEngine.INSTANCE.initialize();
    BookmarkManager.loadBookmarks();
    TtsPlayer.INSTANCE.initialize(mContext);
    TrafficManager.INSTANCE.initialize();
    mSubwayManager.initialize();
    mIsolinesManager.initialize();
    ProcessLifecycleOwner.get().getLifecycle().addObserver(this);

    Logger.i(TAG, "Framework initialized");
    mFrameworkInitialized = true;
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
    nativeAddLocalization("open_in_app", mContext.getString(R.string.open_in_app));
    nativeAddLocalization("postal_code", mContext.getString(R.string.postal_code));
    nativeAddLocalization("wifi", mContext.getString(R.string.category_wifi));
    nativeAddLocalization("share_my_position", mContext.getString(R.string.share_my_position));
    nativeAddLocalization("share_open_in_om_or_browser", mContext.getString(R.string.share_open_in_om_or_browser));
    nativeAddLocalization("share_get_om", mContext.getString(R.string.share_get_om));
  }

  private static native void nativeSetSettingsDir(String settingsPath);

  private static native void nativeInitPlatform(Context context, String apkPath, String writablePath,
                                                String privatePath, String tmpPath, String flavorName, String buildType,
                                                boolean isTablet);

  private static native void nativeInitFramework(@NonNull Runnable onComplete);

  private static native void nativeAddLocalization(String name, String value);

  private static native void nativeOnEnterForeground();

  static
  {
    System.loadLibrary("organicmaps");
  }
}
