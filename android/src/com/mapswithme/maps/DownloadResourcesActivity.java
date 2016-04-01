package com.mapswithme.maps;

import android.annotation.SuppressLint;
import android.content.ContentResolver;
import android.content.Intent;
import android.graphics.Color;
import android.location.Location;
import android.net.Uri;
import android.os.Bundle;
import android.support.annotation.StringRes;
import android.text.TextUtils;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.List;

import com.mapswithme.maps.MwmActivity.MapTask;
import com.mapswithme.maps.MwmActivity.OpenUrlTask;
import com.mapswithme.maps.api.Const;
import com.mapswithme.maps.api.ParsedMwmRequest;
import com.mapswithme.maps.base.BaseMwmFragmentActivity;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.downloader.CountryItem;
import com.mapswithme.maps.downloader.MapManager;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.search.SearchEngine;
import com.mapswithme.util.ConnectionState;
import com.mapswithme.util.Constants;
import com.mapswithme.util.StringUtils;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.Yota;
import com.mapswithme.util.concurrency.ThreadPool;
import com.mapswithme.util.statistics.Statistics;

@SuppressLint("StringFormatMatches")
public class DownloadResourcesActivity extends BaseMwmFragmentActivity
{
  private static final String TAG = DownloadResourcesActivity.class.getName();

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

  private boolean mIsReadingAttachment;
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
      new OpenCountryTaskProcessor(),
      new KmzKmlProcessor()
  };

  private final LocationHelper.LocationListener mLocationListener = new LocationHelper.SimpleLocationListener()
  {
    @Override
    public void onLocationUpdated(Location l)
    {
      if (mCurrentCountry != null)
        return;

      final double lat = l.getLatitude();
      final double lon = l.getLongitude();
      mCurrentCountry = MapManager.nativeFindCountry(lat, lon);
      if (TextUtils.isEmpty(mCurrentCountry))
      {
        mCurrentCountry = null;
        return;
      }

      CountryItem item = CountryItem.fill(mCurrentCountry);

      UiUtils.show(mTvLocation);

      if (item.status == CountryItem.STATUS_DONE)
        mTvLocation.setText(String.format(getString(R.string.download_location_map_up_to_date), item.name));
      else
      {
        final CheckBox checkBox = (CheckBox) findViewById(R.id.chb__download_country);
        UiUtils.show(checkBox);

        String locationText;
        String checkBoxText;

        if (item.status == CountryItem.STATUS_UPDATABLE)
        {
          locationText = getString(R.string.download_location_update_map_proposal);
          checkBoxText = String.format(getString(R.string.update_country_ask), item.name);
        }
        else
        {
          locationText = getString(R.string.download_location_map_proposal);
          checkBoxText = String.format(getString(R.string.download_country_ask), item.name);
        }

        mTvLocation.setText(locationText);
        checkBox.setText(checkBoxText);
      }

      LocationHelper.INSTANCE.removeLocationListener(this);
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
          MapManager.showError(DownloadResourcesActivity.this, item);
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

  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);

    Utils.keepScreenOn(true, getWindow());
    suggestRemoveLiteOrSamsung();
    dispatchIntent();
    setContentView(R.layout.activity_download_resources);
    initViewsAndListeners();

    if (prepareFilesDownload())
    {
      setAction(DOWNLOAD);

      if (ConnectionState.isWifiConnected())
        onDownloadClicked();
    }
  }

  @Override
  protected void onDestroy()
  {
    super.onDestroy();
    if (mCountryDownloadListenerSlot != 0)
    {
      MapManager.nativeUnsubscribe(mCountryDownloadListenerSlot);
      mCountryDownloadListenerSlot = 0;
    }
  }

  @Override
  protected void onResume()
  {
    super.onResume();
    LocationHelper.INSTANCE.addLocationListener(mLocationListener, true);
  }

  @Override
  protected void onPause()
  {
    super.onPause();
    LocationHelper.INSTANCE.removeLocationListener(mLocationListener);
  }

  private void suggestRemoveLiteOrSamsung()
  {
    if (!Yota.isFirstYota() &&
        (Utils.isPackageInstalled(Constants.Package.MWM_LITE_PACKAGE) || Utils.isPackageInstalled(Constants.Package.MWM_SAMSUNG_PACKAGE)))
      Toast.makeText(this, R.string.suggest_uninstall_lite, Toast.LENGTH_LONG).show();
  }

  private void setDownloadMessage(int bytesToDownload)
  {
    mTvMessage.setText(getString(R.string.download_resources, StringUtils.getFileSizeString(bytesToDownload)));
  }

  private boolean prepareFilesDownload()
  {
    final int bytes = nativeGetBytesToDownload();

    if (bytes == 0)
    {
      mAreResourcesDownloaded = true;
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
    if (prepareFilesDownload())
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
    if (mIsReadingAttachment || !mAreResourcesDownloaded)
      return;

    final Intent intent = new Intent(this, MwmActivity.class);

    // Disable animation because MwmActivity should appear exactly over this one
    intent.addFlags(Intent.FLAG_ACTIVITY_NO_ANIMATION | Intent.FLAG_ACTIVITY_CLEAR_TOP);

    // Add saved task to forward to map activity.
    if (mMapTaskToForward != null)
    {
      intent.putExtra(MwmActivity.EXTRA_TASK, mMapTaskToForward);
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

    for (final IntentProcessor ip : mIntentProcessors)
      if (ip.isSupported(intent))
      {
        ip.process(intent);
        return true;
      }

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
      Log.i(TAG, "Query = " + url);
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
      Log.i(TAG, "URL = " + url);
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
      Log.i(TAG, "URL = " + data.toString());

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
      Log.i(TAG, "URL = " + url);
      mMapTaskToForward = new OpenUrlTask(url);
      org.alohalytics.Statistics.logEvent("GoogleMapsIntentProcessor::process", url);
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
      mIsReadingAttachment = true;
      ThreadPool.getStorage().execute(new Runnable()
      {
        @Override
        public void run()
        {
          final boolean result = readKmzFromIntent();
          runOnUiThread(new Runnable()
          {
            @Override
            public void run()
            {
              Utils.toastShortcut(DownloadResourcesActivity.this, result ? R.string.load_kmz_successful : R.string.load_kmz_failed);
              mIsReadingAttachment = false;
              showMap();
            }
          });
        }
      });
      return true;
    }

    private boolean readKmzFromIntent()
    {
      String path = null;
      File tmpFile = null;
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
            final String filePath = MwmApplication.get().getTempPath() + "Attachment" + ext;

            tmpFile = new File(filePath);
            output = new FileOutputStream(tmpFile);
            input = resolver.openInputStream(mData);

            final byte buffer[] = new byte[Constants.MB / 2];
            int read;
            while ((read = input.read(buffer)) != -1)
              output.write(buffer, 0, read);
            output.flush();

            path = filePath;
          }
        } catch (final Exception ex)
        {
          Log.w(TAG, "Attachment not found or io error: " + ex);
        } finally
        {
          Utils.closeStream(input);
          Utils.closeStream(output);
        }
      }
      else
        path = mData.getPath();

      boolean result = false;
      if (path != null)
      {
        Log.d(TAG, "Loading bookmarks file from: " + path);
        result = BookmarkManager.nativeLoadKmzFile(path);
      }
      else
        Log.w(TAG, "Can't get bookmarks file from URI: " + mData);

      if (tmpFile != null)
        //noinspection ResultOfMethodCallIgnored
        tmpFile.delete();

      return result;
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

  private static native int nativeGetBytesToDownload();
  private static native int nativeStartNextFileDownload(Listener listener);
  private static native void nativeCancelCurrentFile();
}
