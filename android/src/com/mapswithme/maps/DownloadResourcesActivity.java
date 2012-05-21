package com.mapswithme.maps;

import java.io.File;

import android.app.Activity;
import android.app.ProgressDialog;
import android.content.Intent;
import android.graphics.Color;
import android.os.Bundle;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.ProgressBar;
import android.widget.TextView;

import com.mapswithme.maps.location.LocationService;

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
  private ProgressBar mProgress = null;
  private Button mDownloadButton = null;
  private Button mPauseButton = null;
  private Button mResumeButton = null;
  private Button mProceedButton = null;
  private Button mTryAgainButton = null;
  private CheckBox mDownloadCountryCheckBox = null;
  private LocationService mLocationService = null;
  private String mCountryName = null;
  private boolean mHasLocation = false;
  private WakeLock mWakeLock = null;

  private int getBytesToDownload()
  {
    return getBytesToDownload(
              mApplication.getApkPath(), 
              mApplication.getDataStoragePath());
  }

  private void disableAutomaticStandby()
  {
    if (mWakeLock == null)
    {
      PowerManager pm = (PowerManager) mApplication.getSystemService(android.content.Context.POWER_SERVICE);
      mWakeLock = pm.newWakeLock(PowerManager.SCREEN_BRIGHT_WAKE_LOCK | PowerManager.ON_AFTER_RELEASE, TAG);
      mWakeLock.acquire();
    }
  }

  private void enableAutomaticStandby()
  {
    if (mWakeLock != null)
    {
      mWakeLock.release();
      mWakeLock = null;
    }
  }

  private void setDownloadMessage(int bytesToDownload)
  {
    Log.d(TAG, "prepareFilesDownload, bytesToDownload:" + bytesToDownload);

    if (bytesToDownload < 1024 * 1024)
      mMsgView.setText(String.format(getString(R.string.download_resources),
                                     bytesToDownload * 1.0f / 1024,
                                     getString(R.string.kb)));
    else
      mMsgView.setText(String.format(getString(R.string.download_resources,
                                               bytesToDownload * 1.0f / 1024 / 1024,
                                               getString(R.string.mb))));

    /// set normal text color
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
      disableAutomaticStandby();
      finishFilesDownload(mBytesToDownload);
    }
  }

  public void onDownloadClicked(View v)
  {
    disableAutomaticStandby();

    mProgress.setVisibility(View.VISIBLE);
    mProgress.setMax(mBytesToDownload);

    mDownloadButton.setVisibility(View.GONE);
    mPauseButton.setVisibility(View.VISIBLE);
    
    startNextFileDownload(this);
  }

  public void onPauseClicked(View v)
  {
    enableAutomaticStandby();
    
    mResumeButton.setVisibility(View.VISIBLE);
    mPauseButton.setVisibility(View.GONE);
    
    cancelCurrentFile();
  }

  public void onResumeClicked(View v)
  {
    disableAutomaticStandby();
    
    mPauseButton.setVisibility(View.VISIBLE);
    mResumeButton.setVisibility(View.GONE);
    
    if (startNextFileDownload(this) == ERR_NO_MORE_FILES)
      finishFilesDownload(ERR_NO_MORE_FILES);
  }

  public void onTryAgainClicked(View v)
  {
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
    enableAutomaticStandby();
    if (result == ERR_NO_MORE_FILES)
    {
      Log.i(TAG, "finished files download");

      if (mHasLocation && mDownloadCountryCheckBox.isChecked())
      {
        mDownloadCountryCheckBox.setVisibility(View.GONE);
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

  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    // Set the same layout as for MWMActivity
    setContentView(R.layout.download_resources);

    mApplication = (MWMApplication)getApplication();
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
      prepareFilesDownload();
    }
  }

  @Override
  protected void onDestroy()
  {
    if (mLocationService != null)
    {
      mLocationService.stopUpdate(this);
      mLocationService = null;
    }

    mMapStorage.unsubscribe(mSlotId);

    super.onDestroy();
  }

  public void onDownloadProgress(int currentTotal, int currentProgress, int globalTotal, int globalProgress)
  {
    Log.d(TAG, "curTotal:" + currentTotal + ", curProgress:" + currentProgress
          + ", glbTotal:" + globalTotal + ", glbProgress:" + globalProgress);

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
  private boolean mReceivedFirstEvent = false;

  private int mLocationsCount = 0;
  private final int mLocationsTryCount = 0;

  @Override
  public void onLocationUpdated(long time, double lat, double lon, float accuracy)
  {
    if (mReceivedFirstEvent)
    {
      if (mLocationsCount == mLocationsTryCount)
      {
        findViewById(R.id.download_resources_location_progress).setVisibility(View.GONE);
        findViewById(R.id.download_resources_location_message).setVisibility(View.GONE);

        CheckBox checkBox = (CheckBox)findViewById(R.id.download_country_checkbox);

        Log.i(TAG, "Searching for country name at location lat=" + lat + ", lon=" + lon);

        checkBox.setVisibility(View.VISIBLE);
        mCountryName = nativeGetCountryName(lat, lon);
        mHasLocation = true;
        checkBox.setText(String.format(getString(R.string.download_country_ask), mCountryName));

        mLocationService.stopUpdate(this);
        mLocationService = null;
      }

      Log.d(TAG, "tryCount:" + mLocationsCount);
      ++mLocationsCount;
    }
  }

  @Override
  public void onCompassUpdated(long time, double magneticNorth, double trueNorth, double accuracy)
  {
  }

  @Override
  public void onLocationStatusChanged(int status)
  {
    if (status == LocationService.FIRST_EVENT)
      mReceivedFirstEvent = true;
  }
  
  private native void moveMaps(String fromFolder, String toFolder);
  private native int getBytesToDownload(String m_apkPath, String m_sdcardPath);
  private native int startNextFileDownload(Object observer);
  private native String findCountryByPos(double lat, double lon);
  private native void cancelCurrentFile();
}
