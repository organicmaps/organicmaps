package com.mapswithme.maps;

import android.app.Activity;
import android.content.pm.PackageManager.NameNotFoundException;
import android.os.Build;
import android.os.Environment;
import android.util.Log;
import android.widget.Toast;

import com.google.android.gms.ads.identifier.AdvertisingIdClient;
import com.google.android.gms.ads.identifier.AdvertisingIdClient.Info;
import com.google.android.gms.common.GooglePlayServicesNotAvailableException;
import com.google.android.gms.common.GooglePlayServicesRepairableException;
import com.mapswithme.maps.MapStorage.Index;
import com.mapswithme.maps.background.Notifier;
import com.mapswithme.maps.background.WorkerService;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.guides.GuideInfo;
import com.mapswithme.maps.guides.GuidesUtils;
import com.mapswithme.util.Constants;
import com.mapswithme.util.FbUtil;
import com.mapswithme.util.Utils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.StubLogger;
import com.mobileapptracker.MobileAppTracker;

import java.io.File;
import java.io.IOException;

public class MWMApplication extends android.app.Application implements MapStorage.Listener
{
  private final static String TAG = "MWMApplication";
  private static final CharSequence PRO_PACKAGE_POSTFIX = ".pro";
  private static final String FOREGROUND_TIME_SETTING = "AllForegroundTime";

  private static MWMApplication mSelf;

  private MapStorage mStorage = null;

  private boolean mIsYota = false;

  // We check how old is modified date of our MapsWithMe folder
  private final static long TIME_DELTA = 5 * 1000;

  private MobileAppTracker mMobileAppTracker = null;
  private final Logger mLogger = StubLogger.get();
//      SimpleLogger.get("MAT");

  public MWMApplication()
  {
    super();
    mSelf = this;
  }

  public static MWMApplication get()
  {
    return mSelf;
  }

  private void showDownloadToast(int resID, Index idx)
  {
    final String msg = String.format(getString(resID), mStorage.countryName(idx));
    Toast.makeText(this, msg, Toast.LENGTH_LONG).show();
  }

  @Override
  public void onCountryStatusChanged(Index idx)
  {
    switch (mStorage.countryStatus(idx))
    {
    case MapStorage.ON_DISK:
      Notifier.placeDownloadCompleted(idx, getMapStorage().countryName(idx));
      tryNotifyGuideAvailable(idx);
      break;

    case MapStorage.DOWNLOAD_FAILED:
      Notifier.placeDownloadFailed(idx, getMapStorage().countryName(idx));
      break;
    }
  }

  private void tryNotifyGuideAvailable(Index idx)
  {
    if (Utils.hasAnyGoogleStoreInstalled())
    {
      final GuideInfo info = Framework.getGuideInfoForIndexWithApiCheck(idx);
      if (info != null && !GuidesUtils.isGuideInstalled(info.mAppId, this)
          && !Framework.wasAdvertised(info.mAppId))
      {
        Notifier.placeGuideAvailable(info.mAppId, info.mTitle, info.mMessage);
        Framework.setWasAdvertised(info.mAppId);
      }
    }
  }

  @Override
  public void onCountryProgress(Index idx, long current, long total)
  {
  }

  @Override
  public void onCreate()
  {
    super.onCreate();

    mIsYota = Build.DEVICE.equals(Constants.DEVICE_YOTAPHONE);

    // http://stackoverflow.com/questions/1440957/httpurlconnection-getresponsecode-returns-1-on-second-invocation
    if (Build.VERSION.SDK_INT <= Build.VERSION_CODES.ECLAIR_MR1)
      System.setProperty("http.keepAlive", "false");

    final String extStoragePath = getDataStoragePath();
    final String extTmpPath = getTempPath();

    // Create folders if they don't exist
    new File(extStoragePath).mkdirs();
    new File(extTmpPath).mkdirs();

    // init native framework
    nativeInit(getApkPath(), extStoragePath, extTmpPath,
        getOBBGooglePath(), BuildConfig.IS_PRO, mIsYota);

    getMapStorage().subscribe(this);

    // init cross-platform strings bundle
    nativeAddLocalization("country_status_added_to_queue", getString(R.string.country_status_added_to_queue));
    nativeAddLocalization("country_status_downloading", getString(R.string.country_status_downloading));
    nativeAddLocalization("country_status_download", getString(R.string.country_status_download));
    nativeAddLocalization("country_status_download_routing", getString(R.string.country_status_download_routing));
    nativeAddLocalization("country_status_download_failed", getString(R.string.country_status_download_failed));
    nativeAddLocalization("try_again", getString(R.string.try_again));
    nativeAddLocalization("not_enough_free_space_on_sdcard", getString(R.string.not_enough_free_space_on_sdcard));
    nativeAddLocalization("dropped_pin", getString(R.string.dropped_pin));
    nativeAddLocalization("my_places", getString(R.string.my_places));
    nativeAddLocalization("my_position", getString(R.string.my_position));
    nativeAddLocalization("routes", getString(R.string.routes));


    // init BookmarkManager (automatically loads bookmarks)
    if (hasBookmarks())
      BookmarkManager.getBookmarkManager(getApplicationContext());

    WorkerService.startActionUpdateAds(this);
  }

  public MapStorage getMapStorage()
  {
    if (mStorage == null)
      mStorage = new MapStorage();

    return mStorage;
  }

  public String getApkPath()
  {
    try
    {
      return getPackageManager().getApplicationInfo(getPackageName(), 0).sourceDir;
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
    // Can't use getExternalCacheDir() here because of API level = 7.
    return getExtAppDirectoryPath(Constants.CACHE_DIR);
  }

  public String getExtAppDirectoryPath(String folder)
  {
    final String storagePath = Environment.getExternalStorageDirectory().getAbsolutePath();
    return storagePath.concat(String.format(Constants.STORAGE_PATH, getPackageName(), folder));
  }

  private String getOBBGooglePath()
  {
    final String storagePath = Environment.getExternalStorageDirectory().getAbsolutePath();
    return storagePath.concat(String.format(Constants.OBB_PATH, getPackageName()));
  }

  /// Check if we have free space on storage (writable path).
  public native boolean hasFreeSpace(long size);

  public double getForegroundTime()
  {
    return nativeGetDouble(FOREGROUND_TIME_SETTING, 0);
  }

  public boolean hasBookmarks()
  {
    return BuildConfig.IS_PRO || mIsYota;
  }

  public boolean isYota()
  {
    return mIsYota;
  }

  static
  {
    System.loadLibrary("mapswithme");
  }

  private native void nativeInit(String apkPath, String storagePath,
                                 String tmpPath, String obbGooglePath,
                                 boolean isPro, boolean isYota);

  public native boolean nativeIsBenchmarking();

  /// @name Dealing with dialogs.
  /// @note Constants should be equal with map/dialog_settings.hpp
  /// @{
  static public final int FACEBOOK = 0;
  static public final int BUYPRO = 1;
  static public final int ROUTING = 3;

  public native boolean shouldShowDialog(int dlg);

  static public final int OK = 0;
  static public final int LATER = 1;
  static public final int NEVER = 2;

  public native void submitDialogResult(int dlg, int res);
  /// @}

  private native void nativeAddLocalization(String name, String value);

  /// Dealing with Settings
  public native boolean nativeGetBoolean(String name, boolean defaultValue);

  public native void nativeSetBoolean(String name, boolean value);

  public native int nativeGetInt(String name, int defaultValue);

  public native void nativeSetInt(String name, int value);

  public native long nativeGetLong(String name, long defaultValue);

  public native void nativeSetLong(String name, long value);

  public native double nativeGetDouble(String name, double defaultValue);

  public native void nativeSetDouble(String name, double value);

  public void onMwmStart(Activity activity)
  {
    FbUtil.activate(activity);
    initMAT(activity);
  }

  public void onMwmResume(Activity activity)
  {
    if (mMobileAppTracker != null)
    {
      // Get source of open for app re-engagement
      mMobileAppTracker.setReferralSources(activity);
      // MAT will not function unless the measureSession call is included
      mMobileAppTracker.measureSession();
    }
  }

  private boolean isNewUser()
  {
    final File mwmDir = new File(getDataStoragePath());
    return !mwmDir.exists() || (System.currentTimeMillis() - mwmDir.lastModified() < TIME_DELTA);
  }

  private void initMAT(Activity activity)
  {
    if (!Utils.hasAnyGoogleStoreInstalled())
    {
      mLogger.d("SKIPPING MAT INIT, DOES NOT HAVE GP");
      return;
    }

    final String advId = getString(R.string.advertiser_id);
    final String convKey = getString(R.string.conversion_key);

    final boolean doTrack = !"FALSE".equalsIgnoreCase(advId);
    if (doTrack)
    {
      MobileAppTracker.init(activity, advId, convKey);
      mMobileAppTracker = MobileAppTracker.getInstance();

      if (!isNewUser())
      {
        mMobileAppTracker.setExistingUser(true);
        mLogger.d("Existing user");
      }

      // Collect Google Play Advertising ID
      new Thread(new Runnable()
      {
        @Override
        public void run()
        {
          // See sample code at http://developer.android.com/google/play-services/id.html
          try
          {
            Info adInfo = AdvertisingIdClient.getAdvertisingIdInfo(getApplicationContext());
            mMobileAppTracker.setGoogleAdvertisingId(adInfo.getId(), adInfo.isLimitAdTrackingEnabled());
            mLogger.d("Got Google User ID");
          } catch (IOException e)
          {
            // Unrecoverable error connecting to Google Play services (e.g.,
            // the old version of the service doesn't support getting AdvertisingId).
            e.printStackTrace();
          } catch (GooglePlayServicesNotAvailableException e)
          {
            // Google Play services is not available entirely.
            // Use ANDROID_ID instead
            // AlexZ: we can't use Android ID from 1st of August, 2014, according to Google policies
            // mMobileAppTracker.setAndroidId(Secure.getString(getContentResolver(), Secure.ANDROID_ID));
            e.printStackTrace();
          } catch (GooglePlayServicesRepairableException e)
          {
            // Encountered a recoverable error connecting to Google Play services.
            e.printStackTrace();
          }
        }
      }).start();
    }
  }
}
