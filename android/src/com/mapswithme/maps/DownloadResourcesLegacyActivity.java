package com.mapswithme.maps;

import android.annotation.SuppressLint;
import android.content.Intent;
import android.graphics.Color;
import android.location.Location;
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
import com.mapswithme.maps.base.BaseMwmFragmentActivity;
import com.mapswithme.maps.downloader.CountryItem;
import com.mapswithme.maps.downloader.MapManager;
import com.mapswithme.maps.intent.Factory;
import com.mapswithme.maps.intent.IntentProcessor;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.location.LocationListener;
import com.mapswithme.util.ConnectionState;
import com.mapswithme.util.Constants;
import com.mapswithme.util.StringUtils;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

import java.util.List;

@SuppressLint("StringFormatMatches")
public class DownloadResourcesLegacyActivity extends BaseMwmFragmentActivity
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.DOWNLOADER);
  private static final String TAG = DownloadResourcesLegacyActivity.class.getName();

  public static final String EXTRA_COUNTRY = "country";

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
  @Nullable
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

  @NonNull
  private final IntentProcessor[] mIntentProcessors = {
      Factory.createGeoIntentProcessor(),
      Factory.createHttpGe0IntentProcessor(),
      Factory.createGe0IntentProcessor(),
      Factory.createMapsWithMeIntentProcessor(),
      Factory.createGoogleMapsIntentProcessor(),
      Factory.createOldLeadUrlProcessor(),
      Factory.createBookmarkCatalogueProcessor(),
      Factory.createOldCoreLinkAdapterProcessor(),
      Factory.createOpenCountryTaskProcessor(),
      Factory.createKmzKmlProcessor(this),
      Factory.createShowOnMapProcessor(),
      Factory.createBuildRouteProcessor()
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
  protected void safeOnCreate(@Nullable Bundle savedInstanceState)
  {
    super.safeOnCreate(savedInstanceState);
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

  public void showMap()
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

  private void dispatchIntent()
  {
    final Intent intent = getIntent();
    if (intent == null)
      return;

    final Intent extra = intent.getParcelableExtra(SplashActivity.EXTRA_INTENT);
    if (extra == null)
      return;

    for (IntentProcessor ip : mIntentProcessors)
    {
      if (ip.isSupported(extra))
      {
        mMapTaskToForward = ip.process(intent);
        break;
      }
    }
  }

  private static native int nativeGetBytesToDownload();
  private static native int nativeStartNextFileDownload(Listener listener);
  private static native void nativeCancelCurrentFile();
}
