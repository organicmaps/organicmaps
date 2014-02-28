package com.mapswithme.maps;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import android.annotation.SuppressLint;
import android.content.ActivityNotFoundException;
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

import com.mapswithme.maps.MWMActivity.MapTask;
import com.mapswithme.maps.MWMActivity.OpenUrlTask;
import com.mapswithme.maps.MapStorage.Index;
import com.mapswithme.maps.api.Const;
import com.mapswithme.maps.api.ParsedMmwRequest;
import com.mapswithme.maps.base.MapsWithMeBaseActivity;
import com.mapswithme.maps.location.LocationService;
import com.mapswithme.util.ConnectionState;
import com.mapswithme.util.Utils;
import com.mapswithme.util.statistics.Statistics;

@SuppressLint("StringFormatMatches")
public class DownloadResourcesActivity extends MapsWithMeBaseActivity
                                       implements LocationService.Listener, MapStorage.Listener
{
  private static final String TAG = "DownloadResourcesActivity";

  // Error codes, should match the same codes in JNI

  private static final int ERR_DOWNLOAD_SUCCESS = 0;
  private static final int ERR_NOT_ENOUGH_MEMORY = -1;
  private static final int ERR_NOT_ENOUGH_FREE_SPACE = -2;
  private static final int ERR_STORAGE_DISCONNECTED = -3;
  private static final int ERR_DOWNLOAD_ERROR = -4;
  private static final int ERR_NO_MORE_FILES = -5;
  private static final int ERR_FILE_IN_PROGRESS = -6;

  private MWMApplication mApplication = null;
  private MapStorage mMapStorage = null;
  private int mSlotId = 0;
  private TextView mMsgView = null;
  private TextView mLocationMsgView = null;
  private ProgressBar mProgress = null;
  private Button mButton = null;
  private CheckBox mDownloadCountryCheckBox = null;
  private LocationService mLocationService = null;
  private Index mCountryIndex = null;

  private MapTask mMapTaskToForward;

  private final IntentProcessor[] mIntentProcessors = {
      new GeoIntentProcessor(),
      new HttpGe0IntentProcessor(),
      new Ge0IntentProcessor(),
      new MapsWithMeIntentProcessor(),
      new GooggleMapsIntentProcessor(),
      new OpenCountryTaskProcessor(),
  };

  private void setDownloadMessage(int bytesToDownload)
  {
    Log.d(TAG, "prepareFilesDownload, bytesToDownload:" + bytesToDownload);

    if (bytesToDownload < 1024 * 1024)
      mMsgView.setText(String.format(getString(R.string.download_resources),
                                     (float)bytesToDownload / 1024,
                                     getString(R.string.kb)));
    else
      mMsgView.setText(String.format(getString(R.string.download_resources,
                                               (float)bytesToDownload / 1024 / 1024,
                                               getString(R.string.mb))));

  }

  private boolean prepareFilesDownload()
  {
    final int bytes = getBytesToDownload();

    // Show map if no any downloading needed.
    if (bytes == 0)
    {
      showMapView();
      return false;
    }

    // Do initialization once.
    if (mMapStorage == null)
      initDownloading();

    if (bytes > 0)
    {
      setDownloadMessage(bytes);

      mProgress.setMax(bytes);
      mProgress.setProgress(0);
    }
    else
    {
      finishFilesDownload(bytes);
    }

    return true;
  }

  private static final int DOWNLOAD = 0;
  private static final int PAUSE = 1;
  private static final int RESUME = 2;
  private static final int TRY_AGAIN = 3;
  private static final int PROCEED_TO_MAP = 4;
  private static final int BTN_COUNT = 5;

  private View.OnClickListener m_btnListeners[] = null;
  private String m_btnNames[] = null;

  private void initDownloading()
  {
    // Get GUI elements and subscribe to map storage (for country downloading).
    mMapStorage = mApplication.getMapStorage();
    mSlotId = mMapStorage.subscribe(this);

    mMsgView = (TextView)findViewById(R.id.download_resources_message);
    mProgress = (ProgressBar)findViewById(R.id.download_resources_progress);
    mButton = (Button)findViewById(R.id.download_resources_button);
    mDownloadCountryCheckBox = (CheckBox)findViewById(R.id.download_country_checkbox);
    mLocationMsgView = (TextView)findViewById(R.id.download_resources_location_message);

    // Initialize button states.
    m_btnListeners = new View.OnClickListener[BTN_COUNT];
    m_btnNames = new String[BTN_COUNT];

    m_btnListeners[DOWNLOAD] = new View.OnClickListener()
    {
      @Override
      public void onClick(View v) { onDownloadClicked(v); }
    };
    m_btnNames[DOWNLOAD] = getString(R.string.download);

    m_btnListeners[PAUSE] = new View.OnClickListener()
    {
      @Override
      public void onClick(View v) { onPauseClicked(v); }
    };
    m_btnNames[PAUSE] = getString(R.string.pause);

    m_btnListeners[RESUME] = new View.OnClickListener()
    {
      @Override
      public void onClick(View v) { onResumeClicked(v); }
    };
    m_btnNames[RESUME] = getString(R.string.continue_download);

    m_btnListeners[TRY_AGAIN] = new View.OnClickListener()
    {
      @Override
      public void onClick(View v) { onTryAgainClicked(v); }
    };
    m_btnNames[TRY_AGAIN] = getString(R.string.try_again);

    m_btnListeners[PROCEED_TO_MAP] = new View.OnClickListener()
    {
      @Override
      public void onClick(View v) { onProceedToMapClicked(v); }
    };
    m_btnNames[PROCEED_TO_MAP] = getString(R.string.download_resources_continue);

    // Start listening the location.
    mLocationService = mApplication.getLocationService();
    mLocationService.startUpdate(this);
  }

  private void setAction(int action)
  {
    mButton.setOnClickListener(m_btnListeners[action]);
    mButton.setText(m_btnNames[action]);
  }

  private void doDownload()
  {
    if (startNextFileDownload(this) == ERR_NO_MORE_FILES)
      finishFilesDownload(ERR_NO_MORE_FILES);
  }

  public void onDownloadClicked(View v)
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
    // Initialize downloading from the beginning.
    if (prepareFilesDownload())
    {
      setAction(PAUSE);
      doDownload();
    }
  }

  private void onProceedToMapClicked(View v)
  {
    showMapView();
  }

  public String getErrorMessage(int res)
  {
    int id;
    switch (res)
    {
    case ERR_NOT_ENOUGH_FREE_SPACE: id = R.string.not_enough_free_space_on_sdcard; break;
    case ERR_STORAGE_DISCONNECTED: id = R.string.disconnect_usb_cable; break;

    case ERR_DOWNLOAD_ERROR:
      if (ConnectionState.isConnected(this))
        id = R.string.download_has_failed;
      else
        id = R.string.no_internet_connection_detected;
      break;

    default: id = R.string.not_enough_memory;
    }

    return getString(id);
  }

  public void showMapView()
  {
    // Continue with Main UI initialization (MWMActivity)
    final Intent mwmActivityIntent = new Intent(this, MWMActivity.class);

    // Disable animation because MWMActivity should appear exactly over this one
    // Intent.FLAG_ACTIVITY_REORDER_TO_FRONT
    mwmActivityIntent.addFlags(Intent.FLAG_ACTIVITY_NO_ANIMATION | Intent.FLAG_ACTIVITY_CLEAR_TOP);

    //add task to forward
    if (mMapTaskToForward != null)
    {
      mwmActivityIntent.putExtra(MWMActivity.EXTRA_TASK, mMapTaskToForward);
      mMapTaskToForward = null;
    }

    startActivity(mwmActivityIntent);

    finish();
  }

  public void finishFilesDownload(int result)
  {
    if (result == ERR_NO_MORE_FILES)
    {
      if (mCountryIndex != null && mDownloadCountryCheckBox.isChecked())
      {
        mDownloadCountryCheckBox.setVisibility(View.GONE);
        mLocationMsgView.setVisibility(View.GONE);
        mMsgView.setText(String.format(getString(R.string.downloading_country_can_proceed),
                                       mMapStorage.countryName(mCountryIndex)));

        mProgress.setMax((int)mMapStorage.countryRemoteSizeInBytes(mCountryIndex));
        mProgress.setProgress(0);

        mMapStorage.downloadCountry(mCountryIndex);

        setAction(PROCEED_TO_MAP);
      }
      else
        showMapView();
    }
    else
    {
      mMsgView.setText(getErrorMessage(result));
      mMsgView.setTextColor(Color.RED);

      setAction(TRY_AGAIN);
    }
  }

  @Override
  public void onCountryStatusChanged(MapStorage.Index idx)
  {
    final int status = mMapStorage.countryStatus(idx);

    if (status == MapStorage.ON_DISK)
      showMapView();
  }

  @Override
  public void onCountryProgress(MapStorage.Index idx, long current, long total)
  {
    // Important check - activity can be destroyed
    // but notifications from downloading thread are coming.
    if (mProgress != null)
      mProgress.setProgress((int)current);
  }

  private Intent getPackageIntent(String s)
  {
    return getPackageManager().getLaunchIntentForPackage(s);
  }

  private boolean checkLiteProPackages(boolean isPro)
  {
    try
    {
      if (!isPro)
      {
        final Intent intent = getPackageIntent("com.mapswithme.maps.pro");
        if (intent != null)
        {
          Log.i(TAG, "Trying to launch pro version");

          startActivity(intent);
          finish();
          return true;
        }
      }
      else
      {
        if (!MWMApplication.get().isYota() &&
            (getPackageIntent("com.mapswithme.maps") != null ||
             getPackageIntent("com.mapswithme.maps.samsung") != null))
        {
          Toast.makeText(this, R.string.suggest_uninstall_lite, Toast.LENGTH_LONG).show();
        }
      }
    }
    catch (final ActivityNotFoundException ex)
    {
      Log.d(TAG, "Intent not found", ex);
    }

    return false;
  }

  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    // Do not turn off the screen while downloading needed resources
    getWindow().addFlags(android.view.WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

    super.onCreate(savedInstanceState);

    mApplication = (MWMApplication)getApplication();
    mApplication.onMwmStart(this);

    final boolean isPro = mApplication.isProVersion();
    if (checkLiteProPackages(isPro))
      return;

    final boolean dispatched = dispatchIntent();
    if (!dispatched)
      parseIntentForKMZFile();

    setContentView(R.layout.download_resources);

    if (prepareFilesDownload())
    {
      setAction(DOWNLOAD);

      if (ConnectionState.getState(this) == ConnectionState.CONNECTED_BY_WIFI)
        onDownloadClicked(mButton);
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

    mime = mime.substring(i+1);
    if (mime.equalsIgnoreCase("kmz"))
      return ".kmz";
    else if (mime.equalsIgnoreCase("kml+xml"))
      return ".kml";
    else
      return null;
  }

  private void parseIntentForKMZFile()
  {
    final Intent intent = getIntent();
    if (intent != null)
    {
      final Uri data = intent.getData();
      if (data != null)
      {
        String path = null;
        File tmpFile = null;
        final String scheme = data.getScheme();
        if (scheme != null && !scheme.equalsIgnoreCase("file"))
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
              final String filePath = mApplication.getTempPath() + "Attachment" + ext;

              tmpFile = new File(filePath);
              output = new FileOutputStream(tmpFile);
              input = resolver.openInputStream(data);

              final byte buffer[] = new byte[512 * 1024];
              int read;
              while ((read = input.read(buffer)) != -1)
                output.write(buffer, 0, read);
              output.flush();

              path = filePath;
            }
          }
          catch (final Exception ex)
          {
            Log.w(TAG, "Attachment not found or io error: " + ex);
          }
          finally
          {
            try
            {
              if (input != null)
                input.close();
              if (output != null)
                output.close();
            }
            catch (final IOException ex)
            {
              Log.w(TAG, "Close stream error: " + ex);
            }
          }
        }
        else
          path = data.getPath();

        boolean success = false;
        if (path != null)
        {
          Log.d(TAG, "Loading bookmarks file from: " + path);
          success = loadKMZFile(path);
        }
        else
          Log.w(TAG, "Can't get bookmarks file from URI: " + data);

        if (tmpFile != null)
          tmpFile.delete();

        Utils.toastShortcut(this, success ? R.string.load_kmz_successful : R.string.load_kmz_failed);
      }
    }
  }

  @Override
  protected void onDestroy()
  {
    super.onDestroy();

    if (mLocationService != null)
    {
      mLocationService.stopUpdate(this);
      mLocationService = null;
    }

    if (mMapStorage != null)
      mMapStorage.unsubscribe(mSlotId);
  }

  @Override
  protected void onPause()
  {
    super.onPause();

    if (mLocationService != null)
      mLocationService.stopUpdate(this);
  }

  @Override
  protected void onResume()
  {
    super.onResume();

    if (mLocationService != null)
      mLocationService.startUpdate(this);
  }

  public void onDownloadProgress(int currentTotal, int currentProgress, int globalTotal, int globalProgress)
  {
    if (mProgress != null)
      mProgress.setProgress(globalProgress);
  }

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
    if (mCountryIndex == null)
    {
      final double lat = l.getLatitude();
      final double lon = l.getLongitude();
      Log.i(TAG, "Searching for country name at location lat=" + lat + ", lon=" + lon);

      mCountryIndex = findIndexByPos(lat, lon);
      if (mCountryIndex != null)
      {
        mLocationMsgView.setVisibility(View.VISIBLE);

        final int countryStatus = mMapStorage.countryStatus(mCountryIndex);
        final String name = mMapStorage.countryName(mCountryIndex);

        if (countryStatus == MapStorage.ON_DISK)
          mLocationMsgView.setText(String.format(getString(R.string.download_location_map_up_to_date), name));
        else
        {
          final CheckBox checkBox = (CheckBox)findViewById(R.id.download_country_checkbox);
          checkBox.setVisibility(View.VISIBLE);

          String msgViewText;
          String checkBoxText;

          if (countryStatus == MapStorage.ON_DISK_OUT_OF_DATE)
          {
            msgViewText = getString(R.string.download_location_update_map_proposal);
            checkBoxText = String.format(getString(R.string.update_country_ask), name);
          }
          else
          {
            msgViewText = getString(R.string.download_location_map_proposal);
            checkBoxText = String.format(getString(R.string.download_country_ask), name);
          }

          mLocationMsgView.setText(msgViewText);
          checkBox.setText(checkBoxText);
        }

        mLocationService.stopUpdate(this);
        mLocationService = null;
      }
    }
  }

  @Override
  public void onCompassUpdated(long time, double magneticNorth, double trueNorth, double accuracy)
  {
  }

  @Override
  public void onLocationError(int errorCode)
  {
  }

  private class GeoIntentProcessor implements IntentProcessor
  {
    @Override
    public boolean isIntentSupported(Intent intent)
    {
      return ("geo".equals(intent.getScheme()) && intent.getData() != null);
    }

    @Override
    public boolean processIntent(Intent intent)
    {
      mMapTaskToForward = new OpenUrlTask(intent.getData().toString());
      return true;
    }
  }

  private class Ge0IntentProcessor implements IntentProcessor
  {
    @Override
    public boolean isIntentSupported(Intent intent)
    {
      return ("ge0".equals(intent.getScheme()) && intent.getData() != null);
    }

    @Override
    public boolean processIntent(Intent intent)
    {
      mMapTaskToForward = new OpenUrlTask(intent.getData().toString());
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
      if (data != null)
      {
        final String ge0Url = "ge0:/" + data.getPath();
        mMapTaskToForward = new OpenUrlTask(ge0Url);
        return true;
      }

      return false;
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
      if (apiUrl != null)
      {
        final ParsedMmwRequest request = ParsedMmwRequest.extractFromIntent(intent, getApplicationContext());
        ParsedMmwRequest.setCurrentRequest(request);
        Statistics.INSTANCE.trackApiCall(request);
        if (!request.isPickPointMode())
          mMapTaskToForward = new OpenUrlTask(apiUrl);
        return true;
      }

      return false;
    }
  }

  private class GooggleMapsIntentProcessor implements IntentProcessor
  {
    @Override
    public boolean isIntentSupported(Intent intent)
    {
      final Uri data = intent.getData();
      return (data != null && "maps.google.com".equals(data.getHost()));
    }

    private String extractCoordinates(String query, Pattern pattern)
    {
      String ll = null;
      if (query != null)
      {
        final Matcher m = pattern.matcher(query);
        if (m.find())
          ll = m.group();
      }
      return ll;
    }

    @Override
    public boolean processIntent(Intent intent)
    {
      final Uri data = intent.getData();
      if (data != null)
      {
        final Pattern pattern = Pattern.compile("(-?\\d+\\.?,?)+");

        String ll = extractCoordinates(data.getQueryParameter("ll"), pattern);
        if (ll == null)
          ll = extractCoordinates(data.getQueryParameter("q"), pattern);
        if (ll != null)
        {
          Log.d(TAG, "URL coordinates: " + ll);
          mMapTaskToForward = new OpenUrlTask("geo://" + ll);
          return true;
        }
      }

      return false;
    }
  }

  public static String EXTRA_COUNTRY_INDEX = ".extra.index";
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
      mMapTaskToForward = new MWMActivity.ShowCountryTask(index);
      return true;
    }
  }

  private native int getBytesToDownload();
  private native boolean isWorldExists(String path);
  private native int startNextFileDownload(Object observer);
  private native Index findIndexByPos(double lat, double lon);
  private native void cancelCurrentFile();
  private native boolean loadKMZFile(String path);
}
