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
import app.organicmaps.sdk.util.SharedPropertiesUtils;
import app.organicmaps.sdk.util.StorageUtils;
import app.organicmaps.sdk.util.log.Logger;
import app.organicmaps.sdk.util.log.LogsManager;
import java.io.IOException;

public final class OrganicMaps implements DefaultLifecycleObserver
{
  private static final String TAG = OrganicMaps.class.getSimpleName();

  @NonNull
  private final String mFlavor;

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

  private volatile boolean mFrameworkInitialized;
  private volatile boolean mPlatformInitialized;

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

  public OrganicMaps(@NonNull Context context, @NonNull String flavor, @NonNull String applicationId, int versionCode,
                     @NonNull String versionName, @NonNull String fileProviderAuthority,
                     @NonNull LocationProviderFactory locationProviderFactory)
  {
    mFlavor = flavor;
    mContext = context.getApplicationContext();
    mPreferences = mContext.getSharedPreferences(context.getString(R.string.pref_file_name), Context.MODE_PRIVATE);

    // Set configuration directory as early as possible.
    // Other methods may explicitly use Config, which requires settingsDir to be set.
    final String settingsPath = StorageUtils.getSettingsPath(mContext);
    if (!StorageUtils.createDirectory(settingsPath))
      throw new AssertionError("Can't create settingsDir " + settingsPath);
    Logger.d(TAG, "Settings path = " + settingsPath);
    nativeSetSettingsDir(settingsPath);

    Config.init(mContext, mPreferences, flavor, applicationId, versionCode, versionName, fileProviderAuthority);
    OsmOAuth.init(mPreferences);
    SharedPropertiesUtils.init(mPreferences);
    LogsManager.INSTANCE.initFileLogging(mContext, mPreferences);

    Android7RootCertificateWorkaround.initializeIfNeeded(mContext);

    Icon.loadDefaultIcons(mContext.getResources(), mContext.getPackageName());

    mSensorHelper = new SensorHelper(mContext);
    mLocationHelper = new LocationHelper(mContext, mSensorHelper, locationProviderFactory);
    mIsolinesManager = new IsolinesManager();
    mSubwayManager = new SubwayManager(mContext);
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
    return mFrameworkInitialized && mPlatformInitialized;
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
    if (mFrameworkInitialized)
      return false;

    nativeInitFramework(onComplete);

    initNativeStrings();
    SearchEngine.INSTANCE.initialize();
    BookmarkManager.loadBookmarks();
    TtsPlayer.INSTANCE.initialize(mContext);
    RoutingController.get().initialize(mLocationHelper);
    TrafficManager.INSTANCE.initialize();
    mSubwayManager.initialize();
    mIsolinesManager.initialize();
    ProcessLifecycleOwner.get().getLifecycle().addObserver(this);

    Logger.i(TAG, "Framework initialized");
    mFrameworkInitialized = true;
    return true;
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
