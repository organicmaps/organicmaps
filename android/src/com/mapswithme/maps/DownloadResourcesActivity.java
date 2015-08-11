package com.mapswithme.maps;

import android.annotation.SuppressLint;
import android.content.ContentResolver;
import android.content.Intent;
import android.graphics.Color;
import android.location.Location;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;

import com.mapswithme.country.StorageOptions;
import com.mapswithme.maps.MapStorage.Index;
import com.mapswithme.maps.MwmActivity.MapTask;
import com.mapswithme.maps.MwmActivity.OpenUrlTask;
import com.mapswithme.maps.api.Const;
import com.mapswithme.maps.api.ParsedMwmRequest;
import com.mapswithme.maps.base.BaseMwmFragmentActivity;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.util.ConnectionState;
import com.mapswithme.util.Constants;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.Yota;
import com.mapswithme.util.statistics.Statistics;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;

@SuppressLint("StringFormatMatches")
public class DownloadResourcesActivity extends BaseMwmFragmentActivity
    implements LocationHelper.LocationListener, MapStorage.Listener
{
  private static final String TAG = DownloadResourcesActivity.class.getName();

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

  private int mStorageSlotId;
  private Index mCountryIndex;
  private MapTask mMapTaskToForward;

  private static final int DOWNLOAD = 0;
  private static final int PAUSE = 1;
  private static final int RESUME = 2;
  private static final int TRY_AGAIN = 3;
  private static final int PROCEED_TO_MAP = 4;
  private static final int BTN_COUNT = 5;

  private View.OnClickListener mBtnListeners[];
  private String mBtnNames[];

  private final IntentProcessor[] mIntentProcessors = {
      new GeoIntentProcessor(),
      new HttpGe0IntentProcessor(),
      new Ge0IntentProcessor(),
      new MapsWithMeIntentProcessor(),
      new GoogleMapsIntentProcessor(),
      new OpenCountryTaskProcessor(),
      new UpdateCountryProcessor()
  };

  public static final String EXTRA_COUNTRY_INDEX = ".extra.index";
  public static final String EXTRA_AUTODOWNLOAD_COUNTRY = ".extra.autodownload";
  public static final String EXTRA_UPDATE_COUNTRIES = ".extra.update.countries";

  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);

    Utils.keepScreenOn(true, getWindow());
    suggestRemoveLiteOrSamsung();

    final boolean dispatched = dispatchIntent();
    if (!dispatched)
      parseIntentForKmzFile();

    setContentView(R.layout.activity_download_resources);
    initViewsAndListeners();
    mStorageSlotId = MapStorage.INSTANCE.subscribe(this);

    if (prepareFilesDownload())
    {
      setAction(DOWNLOAD);

      if (ConnectionState.isWifiConnected())
        onDownloadClicked(mBtnDownload);
    }
  }

  @Override
  protected void onResume()
  {
    super.onResume();

    LocationHelper.INSTANCE.addLocationListener(this);
  }

  @Override
  protected void onPause()
  {
    super.onPause();

    LocationHelper.INSTANCE.removeLocationListener(this);
  }

  @Override
  protected void onDestroy()
  {
    super.onDestroy();

    mProgress = null;
    MapStorage.INSTANCE.unsubscribe(mStorageSlotId);
  }

  // TODO change communication with native thread so that it does not send us callbacks after activity destroy
  @SuppressWarnings("unused")
  public void onDownloadProgress(int currentTotal, int currentProgress, int globalTotal, int globalProgress)
  {
    if (mProgress != null)
      mProgress.setProgress(globalProgress);
  }

  // TODO change communication with native thread so that it does not send us callbacks after activity destroy
  @SuppressWarnings("unused")
  public void onDownloadFinished(int errorCode)
  {
    if (errorCode == ERR_DOWNLOAD_SUCCESS)
    {
      final int res = startNextFileDownload(this);
      if (res == ERR_NO_MORE_FILES)
        finishFilesDownload(res);
    }
    else
      finishFilesDownload(errorCode);
  }

  @Override
  public void onLocationUpdated(final Location l)
  {
    if (mCountryIndex != null)
      return;

    final double lat = l.getLatitude();
    final double lon = l.getLongitude();
    Log.i(TAG, "Searching for country name at location lat=" + lat + ", lon=" + lon);

    mCountryIndex = Framework.nativeGetCountryIndex(lat, lon);
    if (mCountryIndex == null)
      return;

    UiUtils.show(mTvLocation);

    final int countryStatus = MapStorage.INSTANCE.countryStatus(mCountryIndex);
    final String name = MapStorage.INSTANCE.countryName(mCountryIndex);

    if (countryStatus == MapStorage.ON_DISK)
      mTvLocation.setText(String.format(getString(R.string.download_location_map_up_to_date), name));
    else
    {
      final CheckBox checkBox = (CheckBox) findViewById(R.id.chb__download_country);
      UiUtils.show(checkBox);

      String locationText;
      String checkBoxText;

      if (countryStatus == MapStorage.ON_DISK_OUT_OF_DATE)
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

    LocationHelper.INSTANCE.removeLocationListener(this);
  }

  @Override
  public void onCompassUpdated(long time, double magneticNorth, double trueNorth, double accuracy) {}

  @Override
  public void onLocationError(int errorCode) {}


  @Override
  public void onCountryStatusChanged(MapStorage.Index idx)
  {
    final int status = MapStorage.INSTANCE.countryStatus(idx);

    if (status == MapStorage.ON_DISK)
      showMap();
  }

  @Override
  public void onCountryProgress(MapStorage.Index idx, long current, long total)
  {
    // Important check - activity can be destroyed
    // but notifications from downloading thread are coming.
    if (mProgress != null)
      mProgress.setProgress((int) current);
  }

  private Intent getPackageIntent(String s)
  {
    return getPackageManager().getLaunchIntentForPackage(s);
  }

  private void suggestRemoveLiteOrSamsung()
  {
    if (!Yota.isFirstYota() &&
        (Utils.isPackageInstalled(Constants.Package.MWM_LITE_PACKAGE) || Utils.isPackageInstalled(Constants.Package.MWM_SAMSUNG_PACKAGE)))
      Toast.makeText(this, R.string.suggest_uninstall_lite, Toast.LENGTH_LONG).show();
  }

  private void setDownloadMessage(int bytesToDownload)
  {
    Log.d(TAG, "prepareFilesDownload, bytesToDownload:" + bytesToDownload);

    if (bytesToDownload < Constants.MB)
      mTvMessage.setText(String.format(getString(R.string.download_resources),
          (float) bytesToDownload / Constants.KB, getString(R.string.kb)));
    else
      mTvMessage.setText(String.format(getString(R.string.download_resources),
          (float) bytesToDownload / Constants.MB, getString(R.string.mb)));

  }

  private boolean prepareFilesDownload()
  {
    final int bytes = getBytesToDownload();

    if (bytes == 0)
    {
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
      public void onClick(View v) { onDownloadClicked(v); }
    };
    mBtnNames[DOWNLOAD] = getString(R.string.download);

    mBtnListeners[PAUSE] = new View.OnClickListener()
    {
      @Override
      public void onClick(View v) { onPauseClicked(v); }
    };
    mBtnNames[PAUSE] = getString(R.string.pause);

    mBtnListeners[RESUME] = new View.OnClickListener()
    {
      @Override
      public void onClick(View v) { onResumeClicked(v); }
    };
    mBtnNames[RESUME] = getString(R.string.continue_download);

    mBtnListeners[TRY_AGAIN] = new View.OnClickListener()
    {
      @Override
      public void onClick(View v) { onTryAgainClicked(v); }
    };
    mBtnNames[TRY_AGAIN] = getString(R.string.try_again);

    mBtnListeners[PROCEED_TO_MAP] = new View.OnClickListener()
    {
      @Override
      public void onClick(View v) { onProceedToMapClicked(v); }
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
    if (startNextFileDownload(this) == ERR_NO_MORE_FILES)
      finishFilesDownload(ERR_NO_MORE_FILES);
  }

  private void onDownloadClicked(View v)
  {
    setAction(PAUSE);
    doDownload();
  }

  private void onPauseClicked(View v)
  {
    setAction(RESUME);
    cancelCurrentFile();
  }

  private void onResumeClicked(View v)
  {
    setAction(PAUSE);
    doDownload();
  }

  private void onTryAgainClicked(View v)
  {
    if (prepareFilesDownload())
    {
      setAction(PAUSE);
      doDownload();
    }
  }

  private void onProceedToMapClicked(View v)
  {
    showMap();
  }

  public String getErrorMessage(int res)
  {
    int id;
    switch (res)
    {
    case ERR_NOT_ENOUGH_FREE_SPACE:
      id = R.string.not_enough_free_space_on_sdcard;
      break;
    case ERR_STORAGE_DISCONNECTED:
      id = R.string.disconnect_usb_cable;
      break;

    case ERR_DOWNLOAD_ERROR:
      if (ConnectionState.isConnected())
        id = R.string.download_has_failed;
      else
        id = R.string.no_internet_connection_detected;
      break;

    default:
      id = R.string.not_enough_memory;
    }

    return getString(id);
  }

  private void showMap()
  {
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
      if (mCountryIndex != null && mChbDownloadCountry.isChecked())
      {
        UiUtils.hide(mChbDownloadCountry, mTvLocation);
        mTvMessage.setText(String.format(getString(R.string.downloading_country_can_proceed),
            MapStorage.INSTANCE.countryName(mCountryIndex)));

        mProgress.setMax((int) MapStorage.INSTANCE.countryRemoteSizeInBytes(mCountryIndex, StorageOptions.MAP_OPTION_MAP_ONLY));
        mProgress.setProgress(0);

        Framework.downloadCountry(mCountryIndex);

        setAction(PROCEED_TO_MAP);
      }
      else
        showMap();
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
    if (intent != null)
    {
      for (final IntentProcessor ip : mIntentProcessors)
      {
        if (ip.isIntentSupported(intent))
        {
          ip.processIntent(intent);
          return true;
        }
      }
    }

    return false;
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

  private void parseIntentForKmzFile()
  {
    final Intent intent = getIntent();
    if (intent == null)
      return;

    final Uri data = intent.getData();
    if (data == null)
      return;

    String path = null;
    File tmpFile = null;
    final String scheme = data.getScheme();
    if (scheme != null && !scheme.equalsIgnoreCase(Constants.Url.DATA_SCHEME_FILE))
    {
      // scheme is "content" or "http" - need to download file first
      InputStream input = null;
      OutputStream output = null;

      try
      {
        final ContentResolver resolver = getContentResolver();
        final String ext = getExtensionFromMime(resolver.getType(data));
        if (ext != null)
        {
          final String filePath = MwmApplication.get().getTempPath() + "Attachment" + ext;

          tmpFile = new File(filePath);
          output = new FileOutputStream(tmpFile);
          input = resolver.openInputStream(data);

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
      path = data.getPath();

    boolean success = false;
    if (path != null)
    {
      Log.d(TAG, "Loading bookmarks file from: " + path);
      success = loadKmzFile(path);
    }
    else
      Log.w(TAG, "Can't get bookmarks file from URI: " + data);

    if (tmpFile != null)
      //noinspection ResultOfMethodCallIgnored
      tmpFile.delete();

    Utils.toastShortcut(this, success ? R.string.load_kmz_successful : R.string.load_kmz_failed);
  }

  private class GeoIntentProcessor implements IntentProcessor
  {
    @Override
    public boolean isIntentSupported(Intent intent)
    {
      return (intent.getData() != null && "geo".equals(intent.getScheme()));
    }

    @Override
    public boolean processIntent(Intent intent)
    {
      final String url = intent.getData().toString();
      Log.i(TAG, "Query = " + url);
      mMapTaskToForward = new OpenUrlTask(url);
      org.alohalytics.Statistics.logEvent("GeoIntentProcessor::processIntent", url);
      return true;
    }
  }

  private class Ge0IntentProcessor implements IntentProcessor
  {
    @Override
    public boolean isIntentSupported(Intent intent)
    {
      return (intent.getData() != null && "ge0".equals(intent.getScheme()));
    }

    @Override
    public boolean processIntent(Intent intent)
    {
      final String url = intent.getData().toString();
      Log.i(TAG, "URL = " + url);
      mMapTaskToForward = new OpenUrlTask(url);
      org.alohalytics.Statistics.logEvent("Ge0IntentProcessor::processIntent", url);
      return true;
    }
  }

  private class HttpGe0IntentProcessor implements IntentProcessor
  {
    @Override
    public boolean isIntentSupported(Intent intent)
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
    public boolean processIntent(Intent intent)
    {
      final Uri data = intent.getData();
      Log.i(TAG, "URL = " + data.toString());

      final String ge0Url = "ge0:/" + data.getPath();
      mMapTaskToForward = new OpenUrlTask(ge0Url);
      org.alohalytics.Statistics.logEvent("HttpGe0IntentProcessor::processIntent", ge0Url);
      return true;
    }
  }

  /**
   * Use this to invoke API task.
   */
  private class MapsWithMeIntentProcessor implements IntentProcessor
  {
    @Override
    public boolean isIntentSupported(Intent intent)
    {
      return Const.ACTION_MWM_REQUEST.equals(intent.getAction());
    }

    @Override
    public boolean processIntent(final Intent intent)
    {
      final String apiUrl = intent.getStringExtra(Const.EXTRA_URL);
      org.alohalytics.Statistics.logEvent("MapsWithMeIntentProcessor::processIntent", apiUrl == null ? "null" : apiUrl);
      if (apiUrl != null)
      {
        Framework.cleanSearchLayerOnMap();

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
    public boolean isIntentSupported(Intent intent)
    {
      final Uri data = intent.getData();
      return (data != null && "maps.google.com".equals(data.getHost()));
    }

    @Override
    public boolean processIntent(Intent intent)
    {
      final String url = intent.getData().toString();
      Log.i(TAG, "URL = " + url);
      mMapTaskToForward = new OpenUrlTask(url);
      org.alohalytics.Statistics.logEvent("GoogleMapsIntentProcessor::processIntent", url);
      return true;
    }
  }

  private class OpenCountryTaskProcessor implements IntentProcessor
  {
    @Override
    public boolean isIntentSupported(Intent intent)
    {
      return intent.hasExtra(EXTRA_COUNTRY_INDEX);
    }

    @Override
    public boolean processIntent(Intent intent)
    {
      final Index index = (Index) intent.getSerializableExtra(EXTRA_COUNTRY_INDEX);
      final boolean autoDownload = intent.getBooleanExtra(EXTRA_AUTODOWNLOAD_COUNTRY, false);
      if (autoDownload)
        Statistics.INSTANCE.trackDownloadCountryNotificationClicked();
      mMapTaskToForward = new MwmActivity.ShowCountryTask(index, autoDownload);
      org.alohalytics.Statistics.logEvent("OpenCountryTaskProcessor::processIntent", new String[]{"autoDownload", String.valueOf(autoDownload)}, LocationHelper.INSTANCE.getLastLocation());
      return true;
    }
  }

  private class UpdateCountryProcessor implements IntentProcessor
  {
    @Override
    public boolean isIntentSupported(Intent intent)
    {
      return intent.getBooleanExtra(EXTRA_UPDATE_COUNTRIES, false);
    }

    @Override
    public boolean processIntent(Intent intent)
    {
      org.alohalytics.Statistics.logEvent("UpdateCountryProcessor::processIntent");
      mMapTaskToForward = new MwmActivity.UpdateCountryTask();
      return true;
    }
  }

  private native int getBytesToDownload();

  private native int startNextFileDownload(Object observer);

  private native void cancelCurrentFile();

  private native boolean loadKmzFile(String path);
}
