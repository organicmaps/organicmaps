package com.mapswithme.maps;

import java.io.File;

import android.app.Activity;
import android.app.ProgressDialog;
import android.content.ActivityNotFoundException;
import android.content.Intent;
import android.graphics.Color;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;

import com.mapswithme.maps.location.LocationService;
import com.mapswithme.util.ConnectionState;

public class DownloadResourcesActivity extends Activity implements LocationService.Listener, MapStorage.Listener
{
  private static final String TAG = "DownloadResourcesActivity";

  private ProgressDialog mDialog = null;

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
  private int mBytesToDownload = 0;
  private int mSlotId = 0;
  private TextView mMsgView = null;
  private TextView mLocationMsgView = null;
  private ProgressBar mProgress = null;
  private Button mDownloadButton = null;
  private Button mPauseButton = null;
  private Button mResumeButton = null;
  private Button mProceedButton = null;
  private Button mTryAgainButton = null;
  private CheckBox mDownloadCountryCheckBox = null;
  private LocationService mLocationService = null;
  private String mCountryName = null;

  private int getBytesToDownload()
  {
    return getBytesToDownload(mApplication.getApkPath(),
                              mApplication.getDataStoragePath());
  }

  private boolean isWorldExists()
  {
    // 2.0.3 version doesn't contain WorldCoasts on sdcard. No need to check something.
    //return isWorldExists(mApplication.getDataStoragePath());
    return false;
  }

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

    // set normal text color
    mMsgView.setTextColor(Color.WHITE);
  }

  protected void prepareFilesDownload()
  {
    if (mBytesToDownload > 0)
    {
      setDownloadMessage(mBytesToDownload);

      findViewById(R.id.download_resources_location_progress).setVisibility(View.VISIBLE);
      findViewById(R.id.download_resources_location_message).setVisibility(View.VISIBLE);

      mLocationService = mApplication.getLocationService();
      mLocationService.startUpdate(this);
    }
    else
    {
      mApplication.disableAutomaticStandby();
      finishFilesDownload(mBytesToDownload);
    }
  }

  public void onDownloadClicked(View v)
  {
    mApplication.disableAutomaticStandby();

    mProgress.setVisibility(View.VISIBLE);
    mProgress.setMax(mBytesToDownload);

    mDownloadButton.setVisibility(View.GONE);
    mPauseButton.setVisibility(View.VISIBLE);

    startNextFileDownload(this);
  }

  public void onPauseClicked(View v)
  {
    mApplication.enableAutomaticStandby();

    mResumeButton.setVisibility(View.VISIBLE);
    mPauseButton.setVisibility(View.GONE);

    cancelCurrentFile();
  }

  public void onResumeClicked(View v)
  {
    mApplication.disableAutomaticStandby();

    mPauseButton.setVisibility(View.VISIBLE);
    mResumeButton.setVisibility(View.GONE);

    if (startNextFileDownload(this) == ERR_NO_MORE_FILES)
      finishFilesDownload(ERR_NO_MORE_FILES);
  }

  public void onTryAgainClicked(View v)
  {
    mApplication.disableAutomaticStandby();

    mProgress.setVisibility(View.VISIBLE);
    mTryAgainButton.setVisibility(View.GONE);
    mPauseButton.setVisibility(View.VISIBLE);

    mBytesToDownload = getBytesToDownload();

    setDownloadMessage(mBytesToDownload);

    if (startNextFileDownload(this) == ERR_NO_MORE_FILES)
      finishFilesDownload(ERR_NO_MORE_FILES);
  }

  public void onProceedToMapClicked(View v)
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
    case ERR_DOWNLOAD_ERROR: id = R.string.download_has_failed; break;
    default: id = R.string.not_enough_memory;
    }

    return getString(id);
  }

  public void showMapView()
  {
    // Continue with Main UI initialization (MWMActivity)
    Intent mwmActivityIntent = new Intent(this, MWMActivity.class);

    // Disable animation because MWMActivity should appear exactly over this one
    mwmActivityIntent.addFlags(Intent.FLAG_ACTIVITY_NO_ANIMATION | Intent.FLAG_ACTIVITY_REORDER_TO_FRONT);
    startActivity(mwmActivityIntent);

    finish();
  }

  public void finishFilesDownload(int result)
  {
    mApplication.enableAutomaticStandby();
    if (result == ERR_NO_MORE_FILES)
    {
      if (mCountryName != null && mDownloadCountryCheckBox.isChecked())
      {
        mDownloadCountryCheckBox.setVisibility(View.GONE);
        mLocationMsgView.setVisibility(View.GONE);
        mMsgView.setText(String.format(getString(R.string.downloading_country_can_proceed),
                                       mCountryName));

        MapStorage.Index idx = mMapStorage.findIndexByName(mCountryName);

        if (idx.isValid())
        {
          mProgress.setMax((int)mMapStorage.countryRemoteSizeInBytes(idx));
          mProgress.setProgress(0);

          mMapStorage.downloadCountry(idx);

          mProceedButton.setVisibility(View.VISIBLE);
          mPauseButton.setVisibility(View.GONE);
        }
        else
          showMapView();
      }
      else
        showMapView();
    }
    else
    {
      mMsgView.setText(getErrorMessage(result));
      mMsgView.setTextColor(Color.RED);

      mPauseButton.setVisibility(View.GONE);
      mDownloadButton.setVisibility(View.GONE);
      mTryAgainButton.setVisibility(View.VISIBLE);
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
        if (getPackageIntent("com.mapswithme.maps") != null)
        {
          Toast.makeText(this, R.string.suggest_uninstall_lite, Toast.LENGTH_LONG).show();
        }
      }
    }
    catch (ActivityNotFoundException ex)
    {
      Log.d(TAG, "Intent not found", ex);
    }

    return false;
  }

  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);

    mApplication = (MWMApplication)getApplication();

    final boolean isPro = mApplication.isProVersion();
    if (checkLiteProPackages(isPro))
      return;

    setContentView(R.layout.download_resources);

    mMapStorage = MapStorage.getInstance();
    mSlotId = mMapStorage.subscribe(this);

    // Create sdcard folder if it doesn't exist
    new File(mApplication.getDataStoragePath()).mkdirs();
    // Used to migrate from v2.0.0 to 2.0.1
    moveMaps(mApplication.getExtAppDirectoryPath("files"),
             mApplication.getDataStoragePath());

    mBytesToDownload = getBytesToDownload();

    if (mBytesToDownload == 0)
      showMapView();
    else
    {
      mMsgView = (TextView)findViewById(R.id.download_resources_message);
      mProgress = (ProgressBar)findViewById(R.id.download_resources_progress);
      mDownloadButton = (Button)findViewById(R.id.download_resources_button_download);
      mPauseButton = (Button)findViewById(R.id.download_resources_button_pause);
      mResumeButton = (Button)findViewById(R.id.download_resources_button_resume);
      mProceedButton = (Button)findViewById(R.id.download_resources_button_proceed_to_map);
      mTryAgainButton = (Button)findViewById(R.id.download_resources_button_tryagain);
      mDownloadCountryCheckBox = (CheckBox)findViewById(R.id.download_country_checkbox);
      mLocationMsgView = (TextView)findViewById(R.id.download_resources_location_message);

      prepareFilesDownload();

      switch (ConnectionState.getState(this))
      {
      case ConnectionState.CONNECTED_BY_WIFI:
        onDownloadClicked(mDownloadButton);
        break;

      case ConnectionState.NOT_CONNECTED:
        if (!isPro && isWorldExists())
          showMapView();
        break;
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
      int res = startNextFileDownload(this);
      if (res == ERR_NO_MORE_FILES)
        finishFilesDownload(res);
    }
    else
      finishFilesDownload(errorCode);
  }

  @Override
  public void onLocationUpdated(long time, double lat, double lon, float accuracy)
  {
    if (mCountryName == null)
    {
      findViewById(R.id.download_resources_location_progress).setVisibility(View.GONE);

      Log.i(TAG, "Searching for country name at location lat=" + lat + ", lon=" + lon);

      mCountryName = findCountryByPos(lat, lon);
      if (mCountryName != null)
      {
        int countryStatus = mMapStorage.countryStatus(mMapStorage.findIndexByName(mCountryName));
        if (countryStatus == MapStorage.ON_DISK)
          mLocationMsgView.setText(String.format(getString(R.string.download_location_map_up_to_date), mCountryName));
        else
        {
          CheckBox checkBox = (CheckBox)findViewById(R.id.download_country_checkbox);
          checkBox.setVisibility(View.VISIBLE);

          String msgViewText;
          String checkBoxText;

          if (countryStatus == MapStorage.ON_DISK_OUT_OF_DATE)
          {
            msgViewText = getString(R.string.download_location_update_map_proposal);
            checkBoxText = String.format(getString(R.string.update_country_ask), mCountryName);
          }
          else
          {
            msgViewText = getString(R.string.download_location_map_proposal);
            checkBoxText = String.format(getString(R.string.download_country_ask), mCountryName);
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
  public void onLocationStatusChanged(int status)
  {
  }

  private native void moveMaps(String fromFolder, String toFolder);
  private native int getBytesToDownload(String apkPath, String sdcardPath);
  private native boolean isWorldExists(String path);
  private native int startNextFileDownload(Object observer);
  private native String findCountryByPos(double lat, double lon);
  private native void cancelCurrentFile();
}
