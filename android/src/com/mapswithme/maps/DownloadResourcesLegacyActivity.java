package com.mapswithme.maps;

import android.annotation.SuppressLint;
import android.content.ContentResolver;
import android.content.Intent;
import android.graphics.Color;
import android.location.Location;
import android.net.Uri;
import android.os.Bundle;
import android.support.annotation.CallSuper;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.annotation.StringRes;
import android.text.TextUtils;
import android.view.View;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;

import com.mapswithme.maps.MwmActivity.MapTask;
import com.mapswithme.maps.MwmActivity.OpenUrlTask;
import com.mapswithme.maps.api.Const;
import com.mapswithme.maps.api.ParsedMwmRequest;
import com.mapswithme.maps.base.BaseMwmFragmentActivity;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.downloader.CountryItem;
import com.mapswithme.maps.downloader.MapManager;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.location.LocationListener;
import com.mapswithme.maps.search.SearchEngine;
import com.mapswithme.util.ConnectionState;
import com.mapswithme.util.Constants;
import com.mapswithme.util.StorageUtils;
import com.mapswithme.util.StringUtils;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.concurrency.ThreadPool;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;
import com.mapswithme.util.statistics.Statistics;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.List;

@SuppressLint("StringFormatMatches")
public class DownloadResourcesLegacyActivity extends BaseMwmFragmentActivity
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.DOWNLOADER);
  private static final String TAG = DownloadResourcesLegacyActivity.class.getName();

  static final String EXTRA_COUNTRY = "country";
  static final String EXTRA_AUTODOWNLOAD = "autodownload";

  // Error codes, should match the same codes in JNI
  private static final int ERR_DOWNLOAD_SUCCESS = 0;
  private static final int ERR_NOT_ENOUGH_MEMORY = -1;
  private static final int ERR_NOT_ENOUGH_FREE_SPACE = -2;
  private static final int ERR_STORAGE_DISCONNECTED = -3;
  private static final int ERR_DOWNLOAD_ERROR = -4;
  private static final int ERR_NO_MORE_FILES = -5;
  private static final int ERR_FILE_IN_PROGRESS = -6;

  private TextView mTvMessage;
  private TextView mTvLocation;
  private ProgressBar mProgress;
  private Button mBtnDownload;
  private CheckBox mChbDownloadCountry;

  private String mCurrentCountry;
  private MapTask mMapTaskToForward;

  private boolean mAreResourcesDownloaded;

  private static final int DOWNLOAD = 0;
  private static final int PAUSE = 1;
  private static final int RESUME = 2;
  private static final int TRY_AGAIN = 3;
  private static final int PROCEED_TO_MAP = 4;
  private static final int BTN_COUNT = 5;

  private View.OnClickListener mBtnListeners[];
  private String mBtnNames[];

  private int mCountryDownloadListenerSlot;

  @SuppressWarnings("unused")
  private interface Listener
  {
    void onProgress(int percent);
    void onFinish(int errorCode);
  }

  private final IntentProcessor[] mIntentProcessors = {
      new GeoIntentProcessor(),
      new HttpGe0IntentProcessor(),
      new Ge0IntentProcessor(),
      new MapsWithMeIntentProcessor(),
      new GoogleMapsIntentProcessor(),
      new OldLeadUrlIntentProcessor(),
      new DeepLinkIntentProcessor(),
      new OpenCountryTaskProcessor(),
      new KmzKmlProcessor(),
      new ShowOnMapProcessor(),
      new BuildRouteProcessor()
  };

  private final LocationListener mLocationListener = new LocationListener.Simple()
  {
    @Override
    public void onLocationUpdated(Location location)
    {
      if (mCurrentCountry != null)
        return;

      final double lat = location.getLatitude();
      final double lon = location.getLongitude();
      mCurrentCountry = MapManager.nativeFindCountry(lat, lon);
      if (TextUtils.isEmpty(mCurrentCountry))
      {
        mCurrentCountry = null;
        return;
      }

      int status = MapManager.nativeGetStatus(mCurrentCountry);
      String name = MapManager.nativeGetName(mCurrentCountry);

      UiUtils.show(mTvLocation);

      if (status == CountryItem.STATUS_DONE)
        mTvLocation.setText(String.format(getString(R.string.download_location_map_up_to_date), name));
      else
      {
        final CheckBox checkBox = (CheckBox) findViewById(R.id.chb__download_country);
        UiUtils.show(checkBox);

        String locationText;
        String checkBoxText;

        if (status == CountryItem.STATUS_UPDATABLE)
        {
          locationText = getString(R.string.download_location_update_map_proposal);
          checkBoxText = String.format(getString(R.string.update_country_ask), name);
        }
        else
        {
          locationText = getString(R.string.download_location_map_proposal);
          checkBoxText = String.format(getString(R.string.download_country_ask), name);
        }

        mTvLocation.setText(locationText);
        checkBox.setText(checkBoxText);
      }

      LocationHelper.INSTANCE.removeListener(this);
    }
  };

  private final Listener mResourcesDownloadListener = new Listener()
  {
    @Override
    public void onProgress(final int percent)
    {
      if (!isFinishing())
        mProgress.setProgress(percent);
    }

    @Override
    public void onFinish(final int errorCode)
    {
      if (isFinishing())
        return;

      if (errorCode == ERR_DOWNLOAD_SUCCESS)
      {
        final int res = nativeStartNextFileDownload(mResourcesDownloadListener);
        if (res == ERR_NO_MORE_FILES)
          finishFilesDownload(res);
      }
      else
        finishFilesDownload(errorCode);
    }
  };

  private final MapManager.StorageCallback mCountryDownloadListener = new MapManager.StorageCallback()
  {
    @Override
    public void onStatusChanged(List<MapManager.StorageCallbackData> data)
    {
      for (MapManager.StorageCallbackData item : data)
      {
        if (!item.isLeafNode)
          continue;

        switch (item.newStatus)
        {
        case CountryItem.STATUS_DONE:
          mAreResourcesDownloaded = true;
          showMap();
          return;

        case CountryItem.STATUS_FAILED:
          MapManager.showError(DownloadResourcesLegacyActivity.this, item, null);
          return;
        }
      }
    }

    @Override
    public void onProgress(String countryId, long localSize, long remoteSize)
    {
      mProgress.setProgress((int)localSize);
    }
  };

  @CallSuper
  @Override
  protected void onCreate(@Nullable Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_download_resources);
    initViewsAndListeners();

    if (prepareFilesDownload(false))
    {
      Utils.keepScreenOn(true, getWindow());
      suggestRemoveLiteOrSamsung();

      setAction(DOWNLOAD);

      if (ConnectionState.isWifiConnected())
        onDownloadClicked();

      return;
    }

    dispatchIntent();
    showMap();
  }

  @Override
  protected void onDestroy()
  {
    super.onDestroy();
    Utils.keepScreenOn(false, getWindow());
    if (mCountryDownloadListenerSlot != 0)
    {
      MapManager.nativeUnsubscribe(mCountryDownloadListenerSlot);
      mCountryDownloadListenerSlot = 0;
    }
  }

  @CallSuper
  @Override
  protected void onResume()
  {
    super.onResume();
    if (!isFinishing())
      LocationHelper.INSTANCE.addListener(mLocationListener, true);
  }

  @Override
  protected void onPause()
  {
    super.onPause();
    LocationHelper.INSTANCE.removeListener(mLocationListener);
  }

  private void suggestRemoveLiteOrSamsung()
  {
    if (Utils.isPackageInstalled(Constants.Package.MWM_LITE_PACKAGE) || Utils.isPackageInstalled(Constants.Package.MWM_SAMSUNG_PACKAGE))
      Toast.makeText(this, R.string.suggest_uninstall_lite, Toast.LENGTH_LONG).show();
  }

  private void setDownloadMessage(int bytesToDownload)
  {
    mTvMessage.setText(getString(R.string.download_resources, StringUtils.getFileSizeString(bytesToDownload)));
  }

  private boolean prepareFilesDownload(boolean showMap)
  {
    final int bytes = nativeGetBytesToDownload();
    if (bytes == 0)
    {
      mAreResourcesDownloaded = true;
      if (showMap)
        showMap();

      return false;
    }

    if (bytes > 0)
    {
      setDownloadMessage(bytes);

      mProgress.setMax(bytes);
      mProgress.setProgress(0);
    }
    else
      finishFilesDownload(bytes);

    return true;
  }

  private void initViewsAndListeners()
  {
    mTvMessage = (TextView) findViewById(R.id.tv__download_message);
    mProgress = (ProgressBar) findViewById(R.id.pb__download_resources);
    mBtnDownload = (Button) findViewById(R.id.btn__download_resources);
    mChbDownloadCountry = (CheckBox) findViewById(R.id.chb__download_country);
    mTvLocation = (TextView) findViewById(R.id.tv__location);

    mBtnListeners = new View.OnClickListener[BTN_COUNT];
    mBtnNames = new String[BTN_COUNT];

    mBtnListeners[DOWNLOAD] = new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        onDownloadClicked();
      }
    };
    mBtnNames[DOWNLOAD] = getString(R.string.download);

    mBtnListeners[PAUSE] = new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        onPauseClicked();
      }
    };
    mBtnNames[PAUSE] = getString(R.string.pause);

    mBtnListeners[RESUME] = new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        onResumeClicked();
      }
    };
    mBtnNames[RESUME] = getString(R.string.continue_download);

    mBtnListeners[TRY_AGAIN] = new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        onTryAgainClicked();
      }
    };
    mBtnNames[TRY_AGAIN] = getString(R.string.try_again);

    mBtnListeners[PROCEED_TO_MAP] = new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        onProceedToMapClicked();
      }
    };
    mBtnNames[PROCEED_TO_MAP] = getString(R.string.download_resources_continue);
  }

  private void setAction(int action)
  {
    mBtnDownload.setOnClickListener(mBtnListeners[action]);
    mBtnDownload.setText(mBtnNames[action]);
  }

  private void doDownload()
  {
    if (nativeStartNextFileDownload(mResourcesDownloadListener) == ERR_NO_MORE_FILES)
      finishFilesDownload(ERR_NO_MORE_FILES);
  }

  private void onDownloadClicked()
  {
    setAction(PAUSE);
    doDownload();
  }

  private void onPauseClicked()
  {
    setAction(RESUME);
    nativeCancelCurrentFile();
  }

  private void onResumeClicked()
  {
    setAction(PAUSE);
    doDownload();
  }

  private void onTryAgainClicked()
  {
    if (prepareFilesDownload(true))
    {
      setAction(PAUSE);
      doDownload();
    }
  }

  private void onProceedToMapClicked()
  {
    mAreResourcesDownloaded = true;
    showMap();
  }

  private static @StringRes int getErrorMessage(int res)
  {
    switch (res)
    {
    case ERR_NOT_ENOUGH_FREE_SPACE:
      return R.string.not_enough_free_space_on_sdcard;

    case ERR_STORAGE_DISCONNECTED:
      return R.string.disconnect_usb_cable;

    case ERR_DOWNLOAD_ERROR:
      return (ConnectionState.isConnected() ? R.string.download_has_failed
                                            : R.string.common_check_internet_connection_dialog);
    default:
      return R.string.not_enough_memory;
    }
  }

  private void showMap()
  {
    if (!mAreResourcesDownloaded)
      return;

    final Intent intent = new Intent(this, MwmActivity.class);

    // Disable animation because MwmActivity should appear exactly over this one
    intent.addFlags(Intent.FLAG_ACTIVITY_NO_ANIMATION | Intent.FLAG_ACTIVITY_CLEAR_TOP);

    // Add saved task to forward to map activity.
    if (mMapTaskToForward != null)
    {
      intent.putExtra(MwmActivity.EXTRA_TASK, mMapTaskToForward);
      intent.putExtra(MwmActivity.EXTRA_LAUNCH_BY_DEEP_LINK,
                      mMapTaskToForward instanceof OpenUrlTask);
      mMapTaskToForward = null;
    }

    startActivity(intent);

    finish();
  }

  private void finishFilesDownload(int result)
  {
    if (result == ERR_NO_MORE_FILES)
    {
      // World and WorldCoasts has been downloaded, we should register maps again to correctly add them to the model and generate indexes etc.
      // TODO fix the hack when separate download of World-s will be removed or refactored
      Framework.nativeDeregisterMaps();
      Framework.nativeRegisterMaps();
      if (mCurrentCountry != null && mChbDownloadCountry.isChecked())
      {
        CountryItem item = CountryItem.fill(mCurrentCountry);

        UiUtils.hide(mChbDownloadCountry, mTvLocation);
        mTvMessage.setText(getString(R.string.downloading_country_can_proceed, item.name));
        mProgress.setMax((int)item.totalSize);
        mProgress.setProgress(0);

        mCountryDownloadListenerSlot = MapManager.nativeSubscribe(mCountryDownloadListener);
        MapManager.nativeDownload(mCurrentCountry);
        setAction(PROCEED_TO_MAP);
      }
      else
      {
        mAreResourcesDownloaded = true;
        showMap();
      }
    }
    else
    {
      mTvMessage.setText(getErrorMessage(result));
      mTvMessage.setTextColor(Color.RED);
      setAction(TRY_AGAIN);
    }
  }

  private boolean dispatchIntent()
  {
    final Intent intent = getIntent();
    if (intent == null)
      return false;

    final Intent extra = intent.getParcelableExtra(SplashActivity.EXTRA_INTENT);
    if (extra == null)
      return false;

    for (final IntentProcessor ip : mIntentProcessors)
      if (ip.isSupported(extra) && ip.process(extra))
        return true;

    return false;
  }

  private class GeoIntentProcessor implements IntentProcessor
  {
    @Override
    public boolean isSupported(Intent intent)
    {
      return (intent.getData() != null && "geo".equals(intent.getScheme()));
    }

    @Override
    public boolean process(Intent intent)
    {
      final String url = intent.getData().toString();
      LOGGER.i(TAG, "Query = " + url);
      mMapTaskToForward = new OpenUrlTask(url);
      org.alohalytics.Statistics.logEvent("GeoIntentProcessor::process", url);
      return true;
    }
  }

  private class Ge0IntentProcessor implements IntentProcessor
  {
    @Override
    public boolean isSupported(Intent intent)
    {
      return (intent.getData() != null && "ge0".equals(intent.getScheme()));
    }

    @Override
    public boolean process(Intent intent)
    {
      final String url = intent.getData().toString();
      LOGGER.i(TAG, "URL = " + url);
      mMapTaskToForward = new OpenUrlTask(url);
      org.alohalytics.Statistics.logEvent("Ge0IntentProcessor::process", url);
      return true;
    }
  }

  private class HttpGe0IntentProcessor implements IntentProcessor
  {
    @Override
    public boolean isSupported(Intent intent)
    {
      if ("http".equalsIgnoreCase(intent.getScheme()))
      {
        final Uri data = intent.getData();
        if (data != null)
          return "ge0.me".equals(data.getHost());
      }

      return false;
    }

    @Override
    public boolean process(Intent intent)
    {
      final Uri data = intent.getData();
      LOGGER.i(TAG, "URL = " + data.toString());

      final String ge0Url = "ge0:/" + data.getPath();
      mMapTaskToForward = new OpenUrlTask(ge0Url);
      org.alohalytics.Statistics.logEvent("HttpGe0IntentProcessor::process", ge0Url);
      return true;
    }
  }

  /**
   * Use this to invoke API task.
   */
  private class MapsWithMeIntentProcessor implements IntentProcessor
  {
    @Override
    public boolean isSupported(Intent intent)
    {
      return Const.ACTION_MWM_REQUEST.equals(intent.getAction());
    }

    @Override
    public boolean process(final Intent intent)
    {
      final String apiUrl = intent.getStringExtra(Const.EXTRA_URL);
      org.alohalytics.Statistics.logEvent("MapsWithMeIntentProcessor::process", apiUrl == null ? "null" : apiUrl);
      if (apiUrl != null)
      {
        SearchEngine.nativeCancelInteractiveSearch();

        final ParsedMwmRequest request = ParsedMwmRequest.extractFromIntent(intent);
        ParsedMwmRequest.setCurrentRequest(request);
        Statistics.INSTANCE.trackApiCall(request);

        if (!ParsedMwmRequest.isPickPointMode())
          mMapTaskToForward = new OpenUrlTask(apiUrl);
        return true;
      }

      return false;
    }
  }

  private class GoogleMapsIntentProcessor implements IntentProcessor
  {
    @Override
    public boolean isSupported(Intent intent)
    {
      final Uri data = intent.getData();
      return (data != null && "maps.google.com".equals(data.getHost()));
    }

    @Override
    public boolean process(Intent intent)
    {
      final String url = intent.getData().toString();
      LOGGER.i(TAG, "URL = " + url);
      mMapTaskToForward = new OpenUrlTask(url);
      org.alohalytics.Statistics.logEvent("GoogleMapsIntentProcessor::process", url);
      return true;
    }
  }

  private class OldLeadUrlIntentProcessor implements IntentProcessor
  {
    @Override
    public boolean isSupported(Intent intent)
    {
      final Uri data = intent.getData();

      if (data == null)
        return false;

      String scheme = intent.getScheme();
      String host = data.getHost();
      if (TextUtils.isEmpty(scheme) || TextUtils.isEmpty(host))
        return false;

      return (scheme.equals("mapsme") || scheme.equals("mapswithme")) && "lead".equals(host);
    }

    @Override
    public boolean process(Intent intent)
    {
      final String url = intent.getData().toString();
      LOGGER.i(TAG, "URL = " + url);
      mMapTaskToForward = new OpenUrlTask(url);
      org.alohalytics.Statistics.logEvent("OldLeadUrlIntentProcessor::process", url);
      return true;
    }
  }

  private class DeepLinkIntentProcessor implements IntentProcessor
  {
    private static final String SCHEME_HTTP = "http";
    private static final String SCHEME_HTTPS = "https";
    private static final String HOST = "dlink.maps.me";
    private static final String SCHEME_CORE = "mapsme";

    @Override
    public boolean isSupported(Intent intent)
    {
      final Uri data = intent.getData();

      if (data == null)
        return false;

      String scheme = intent.getScheme();
      String host = data.getHost();
      if (TextUtils.isEmpty(scheme) || TextUtils.isEmpty(host))
        return false;

      return (scheme.equals(SCHEME_HTTP) || scheme.equals(SCHEME_HTTPS)) && HOST.equals(host);
    }

    @Override
    public boolean process(Intent intent)
    {
      String url = intent.getData().toString();
      LOGGER.i(TAG, "HTTP deeplink = " + url);
      // Transform deeplink to the core expected format,
      // i.e http(s)://host/path?query -> mapsme://path?query.
      url = url.replace(SCHEME_HTTPS, SCHEME_CORE)
               .replace(SCHEME_HTTP, SCHEME_CORE)
               .replace(HOST, "");

      LOGGER.i(TAG, "MAPSME URL = " + url);
      mMapTaskToForward = new OpenUrlTask(url);
      org.alohalytics.Statistics.logEvent(this.getClass().getSimpleName() + "::process", url);
      return true;
    }
  }

  private class OpenCountryTaskProcessor implements IntentProcessor
  {
    @Override
    public boolean isSupported(Intent intent)
    {
      return intent.hasExtra(EXTRA_COUNTRY);
    }

    @Override
    public boolean process(Intent intent)
    {
      String countryId = intent.getStringExtra(EXTRA_COUNTRY);
      final boolean autoDownload = intent.getBooleanExtra(EXTRA_AUTODOWNLOAD, false);
      if (autoDownload)
        Statistics.INSTANCE.trackEvent(Statistics.EventName.DOWNLOAD_COUNTRY_NOTIFICATION_CLICKED);

      mMapTaskToForward = new MwmActivity.ShowCountryTask(countryId, autoDownload);
      org.alohalytics.Statistics.logEvent("OpenCountryTaskProcessor::process",
                                          new String[] { "autoDownload", String.valueOf(autoDownload) },
                                          LocationHelper.INSTANCE.getSavedLocation());
      return true;
    }
  }

  private class KmzKmlProcessor implements IntentProcessor
  {
    private Uri mData;

    @Override
    public boolean isSupported(Intent intent)
    {
      mData = intent.getData();
      return mData != null;
    }

    @Override
    public boolean process(Intent intent)
    {
      ThreadPool.getStorage().execute(new Runnable()
      {
        @Override
        public void run()
        {
          readKmzFromIntent();
          runOnUiThread(new Runnable()
          {
            @Override
            public void run()
            {
              showMap();
            }
          });
        }
      });
      return true;
    }

    private void readKmzFromIntent()
    {
      String path = null;
      boolean isTemporaryFile = false;
      final String scheme = mData.getScheme();
      if (scheme != null && !scheme.equalsIgnoreCase(ContentResolver.SCHEME_FILE))
      {
        // scheme is "content" or "http" - need to download or read file first
        InputStream input = null;
        OutputStream output = null;

        try
        {
          final ContentResolver resolver = getContentResolver();
          final String ext = getExtensionFromMime(resolver.getType(mData));
          if (ext != null)
          {
            final String filePath = StorageUtils.getTempPath() + "Attachment" + ext;

            File tmpFile = new File(filePath);
            output = new FileOutputStream(tmpFile);
            input = resolver.openInputStream(mData);

            final byte buffer[] = new byte[Constants.MB / 2];
            int read;
            while ((read = input.read(buffer)) != -1)
              output.write(buffer, 0, read);
            output.flush();

            path = filePath;
            isTemporaryFile = true;
          }
        } catch (final Exception ex)
        {
          LOGGER.w(TAG, "Attachment not found or io error: " + ex, ex);
        } finally
        {
          Utils.closeStream(input);
          Utils.closeStream(output);
        }
      }
      else
        path = mData.getPath();

      if (path != null)
      {
        LOGGER.d(TAG, "Loading bookmarks file from: " + path);
        BookmarkManager.loadKmzFile(path, isTemporaryFile);
      }
      else
        LOGGER.w(TAG, "Can't get bookmarks file from URI: " + mData);
    }

    private String getExtensionFromMime(String mime)
    {
      final int i = mime.lastIndexOf('.');
      if (i == -1)
        return null;

      mime = mime.substring(i + 1);
      if (mime.equalsIgnoreCase("kmz"))
        return ".kmz";
      else if (mime.equalsIgnoreCase("kml+xml"))
        return ".kml";
      else
        return null;
    }
  }

  private class ShowOnMapProcessor implements IntentProcessor
  {
    private static final String ACTION_SHOW_ON_MAP = "com.mapswithme.maps.pro.action.SHOW_ON_MAP";
    private static final String EXTRA_LAT = "lat";
    private static final String EXTRA_LON = "lon";

    @Override
    public boolean isSupported(Intent intent)
    {
      return ACTION_SHOW_ON_MAP.equals(intent.getAction());
    }

    @Override
    public boolean process(Intent intent)
    {
      if (!intent.hasExtra(EXTRA_LAT) || !intent.hasExtra(EXTRA_LON))
        return false;

      double lat = getCoordinateFromIntent(intent, EXTRA_LAT);
      double lon = getCoordinateFromIntent(intent, EXTRA_LON);
      mMapTaskToForward = new MwmActivity.ShowPointTask(lat, lon);

      return true;
    }
  }

  private class BuildRouteProcessor implements IntentProcessor
  {
    private static final String ACTION_BUILD_ROUTE = "com.mapswithme.maps.pro.action.BUILD_ROUTE";
    private static final String EXTRA_LAT_TO = "lat_to";
    private static final String EXTRA_LON_TO = "lon_to";
    private static final String EXTRA_LAT_FROM = "lat_from";
    private static final String EXTRA_LON_FROM = "lon_from";
    private static final String EXTRA_SADDR = "saddr";
    private static final String EXTRA_DADDR = "daddr";
    private static final String EXTRA_ROUTER = "router";

    @Override
    public boolean isSupported(Intent intent)
    {
      return ACTION_BUILD_ROUTE.equals(intent.getAction());
    }

    @Override
    public boolean process(Intent intent)
    {
      if (!intent.hasExtra(EXTRA_LAT_TO) || !intent.hasExtra(EXTRA_LON_TO))
        return false;

      String saddr = intent.getStringExtra(EXTRA_SADDR);
      String daddr = intent.getStringExtra(EXTRA_DADDR);
      double latTo = getCoordinateFromIntent(intent, EXTRA_LAT_TO);
      double lonTo = getCoordinateFromIntent(intent, EXTRA_LON_TO);
      boolean hasFrom = intent.hasExtra(EXTRA_LAT_FROM) && intent.hasExtra(EXTRA_LON_FROM);
      boolean hasRouter = intent.hasExtra(EXTRA_ROUTER);

      if (hasFrom && hasRouter)
      {
        double latFrom = getCoordinateFromIntent(intent, EXTRA_LAT_FROM);
        double lonFrom = getCoordinateFromIntent(intent, EXTRA_LON_FROM);
        mMapTaskToForward = new MwmActivity.BuildRouteTask(latTo, lonTo, saddr, latFrom,lonFrom,
                                                           daddr, intent.getStringExtra(EXTRA_ROUTER));
      }
      else if (hasFrom)
      {
        double latFrom = getCoordinateFromIntent(intent, EXTRA_LAT_FROM);
        double lonFrom = getCoordinateFromIntent(intent, EXTRA_LON_FROM);
        mMapTaskToForward = new MwmActivity.BuildRouteTask(latTo, lonTo, saddr,
                                                           latFrom,lonFrom, daddr);
      }
      else
      {
        mMapTaskToForward = new MwmActivity.BuildRouteTask(latTo, lonTo,
                                                           intent.getStringExtra(EXTRA_ROUTER));
      }

      return true;
    }
  }

  private static double getCoordinateFromIntent(@NonNull Intent intent, @NonNull String key)
  {
    double value = intent.getDoubleExtra(key, 0.0);
    if (Double.compare(value, 0.0) == 0)
      value = intent.getFloatExtra(key, 0.0f);

    return value;
  }

  private static native int nativeGetBytesToDownload();
  private static native int nativeStartNextFileDownload(Listener listener);
  private static native void nativeCancelCurrentFile();
}
