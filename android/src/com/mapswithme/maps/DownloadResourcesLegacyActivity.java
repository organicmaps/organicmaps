package com.mapswithme.maps;

import android.annotation.SuppressLint;
import android.content.Intent;
import android.location.Location;
import android.os.Bundle;
import android.text.TextUtils;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.ProgressBar;
import android.widget.TextView;

import androidx.annotation.CallSuper;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StringRes;

import com.mapswithme.maps.api.ParsedMwmRequest;
import com.mapswithme.maps.base.BaseMwmFragmentActivity;
import com.mapswithme.maps.dialog.AlertDialog;
import com.mapswithme.maps.dialog.AlertDialogCallback;
import com.mapswithme.maps.downloader.CountryItem;
import com.mapswithme.maps.downloader.MapManager;
import com.mapswithme.maps.intent.Factory;
import com.mapswithme.maps.intent.IntentProcessor;
import com.mapswithme.maps.intent.MapTask;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.location.LocationListener;
import com.mapswithme.util.ConnectionState;
import com.mapswithme.util.CrashlyticsUtils;
import com.mapswithme.util.StringUtils;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.log.Logger;

import java.util.List;

@SuppressLint("StringFormatMatches")
public class DownloadResourcesLegacyActivity extends BaseMwmFragmentActivity implements AlertDialogCallback
{
  private static final String TAG = DownloadResourcesLegacyActivity.class.getSimpleName();

  private static final String ERROR_LOADING_DIALOG_TAG = "error_loading_dialog";
  private static final int ERROR_LOADING_DIALOG_REQ_CODE = 234;

  private static final int REQ_CODE_API_RESULT = 10;

  public static final String EXTRA_COUNTRY = "country";

  // Error codes, should match the same codes in JNI
  private static final int ERR_DOWNLOAD_SUCCESS = 0;
  private static final int ERR_DISK_ERROR = -1;
  private static final int ERR_NOT_ENOUGH_FREE_SPACE = -2;
  private static final int ERR_STORAGE_DISCONNECTED = -3;
  private static final int ERR_DOWNLOAD_ERROR = -4;
  private static final int ERR_NO_MORE_FILES = -5;
  private static final int ERR_FILE_IN_PROGRESS = -6;

  private TextView mTvMessage;
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

  private View.OnClickListener[] mBtnListeners;
  private String[] mBtnNames;

  private int mCountryDownloadListenerSlot;

  @SuppressWarnings("unused")
  private interface Listener
  {
    void onProgress(int percent);
    void onFinish(int errorCode);
  }

  @NonNull
  private final IntentProcessor[] mIntentProcessors = {
      new Factory.GeoIntentProcessor(),
      new Factory.HttpGeoIntentProcessor(),
      new Factory.HttpMapsIntentProcessor(),
      new Factory.OpenCountryTaskProcessor(),
      new Factory.KmzKmlProcessor(this),
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

      if (status != CountryItem.STATUS_DONE)
      {
        UiUtils.show(mChbDownloadCountry);
        String checkBoxText;
        if (status == CountryItem.STATUS_UPDATABLE)
          checkBoxText = String.format(getString(R.string.update_country_ask), name);
        else
          checkBoxText = String.format(getString(R.string.download_country_ask), name);

        mChbDownloadCountry.setText(checkBoxText);
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
  protected void onSafeCreate(@Nullable Bundle savedInstanceState)
  {
    super.onSafeCreate(savedInstanceState);
    setContentView(R.layout.activity_download_resources);
    initViewsAndListeners();

    if (prepareFilesDownload(false))
    {
      Utils.keepScreenOn(true, getWindow());

      setAction(DOWNLOAD);

      return;
    }

    mMapTaskToForward = processIntent();
    showMap();
  }

  @CallSuper
  @Override
  protected void onSafeDestroy()
  {
    super.onSafeDestroy();
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
      LocationHelper.INSTANCE.addListener(mLocationListener);
  }

  @Override
  protected void onPause()
  {
    super.onPause();
    LocationHelper.INSTANCE.removeListener(mLocationListener);
  }

  private void setDownloadMessage(int bytesToDownload)
  {
    mTvMessage.setText(getString(R.string.download_resources,
                                 StringUtils.getFileSizeString(this, bytesToDownload)));
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
    mTvMessage = findViewById(R.id.download_message);
    mProgress = findViewById(R.id.progressbar);
    mBtnDownload = findViewById(R.id.btn_download_resources);
    mChbDownloadCountry = findViewById(R.id.chb_download_country);

    mBtnListeners = new View.OnClickListener[BTN_COUNT];
    mBtnNames = new String[BTN_COUNT];

    mBtnListeners[DOWNLOAD] = v -> onDownloadClicked();
    mBtnNames[DOWNLOAD] = getString(R.string.download);

    mBtnListeners[PAUSE] = v -> onPauseClicked();
    mBtnNames[PAUSE] = getString(R.string.pause);

    mBtnListeners[RESUME] = v -> onResumeClicked();
    mBtnNames[RESUME] = getString(R.string.continue_download);

    mBtnListeners[TRY_AGAIN] = v -> onTryAgainClicked();
    mBtnNames[TRY_AGAIN] = getString(R.string.try_again);

    mBtnListeners[PROCEED_TO_MAP] = v -> onProceedToMapClicked();
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
      return (ConnectionState.INSTANCE.isConnected() ? R.string.download_has_failed
                                            : R.string.common_check_internet_connection_dialog);
    default:
      assert(res == ERR_DISK_ERROR);
      return R.string.disk_error;
    }
  }

  @StringRes
  private static int getErrorTitle(int res)
  {
    switch (res)
    {
      case ERR_NOT_ENOUGH_FREE_SPACE:
        return R.string.routing_not_enough_space;
      case ERR_STORAGE_DISCONNECTED:
        return R.string.disconnect_usb_cable_title;
      case ERR_DOWNLOAD_ERROR:
        return R.string.connection_failure;
      default:
        assert(res == ERR_DISK_ERROR);
        return R.string.disk_error_title;
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
      intent.putExtra(MwmActivity.EXTRA_LAUNCH_BY_DEEP_LINK, true);
      mMapTaskToForward = null;

      if (ParsedMwmRequest.getCurrentRequest() != null)
      {
        // Wait for the result from MwmActivity for API callers.
        startActivityForResult(intent, REQ_CODE_API_RESULT);
        return;
      }
    }

    startActivity(intent);
    finish();
  }

  protected void onActivityResult(int requestCode, int resultCode, Intent data)
  {
    super.onActivityResult(requestCode, resultCode, data);
    if (requestCode == REQ_CODE_API_RESULT)
    {
      setResult(resultCode, data);
      finish();
    }
  }

  private void finishFilesDownload(int result)
  {
    if (result == ERR_NO_MORE_FILES)
    {
      // World and WorldCoasts has been downloaded, we should register maps again to correctly add them to the model.
      Framework.nativeReloadWorldMaps();

      if (mCurrentCountry != null && mChbDownloadCountry.isChecked())
      {
        CountryItem item = CountryItem.fill(mCurrentCountry);
        UiUtils.hide(mChbDownloadCountry);
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
        mMapTaskToForward = processIntent();
        showMap();
      }
    }
    else
    {
      showErrorDialog(result);
    }
  }

  @Nullable
  private MapTask processIntent()
  {
    final Intent intent = getIntent();
    if (intent == null)
      return null;

    String msg = "Incoming intent uri: " + intent;
    Logger.i(TAG, msg);
    CrashlyticsUtils.INSTANCE.log(Log.INFO, TAG, msg);

    MapTask mapTaskToForward;
    for (IntentProcessor ip : mIntentProcessors)
    {
      if ((mapTaskToForward = ip.process(intent)) != null)
        return mapTaskToForward;
    }

    return null;
  }

  private void showErrorDialog(int result)
  {
    AlertDialog dialog = new AlertDialog.Builder()
        .setTitleId(getErrorTitle(result))
        .setMessageId(getErrorMessage(result))
        .setPositiveBtnId(R.string.try_again)
        .setReqCode(ERROR_LOADING_DIALOG_REQ_CODE)
        .setFragManagerStrategyType(AlertDialog.FragManagerStrategyType.ACTIVITY_FRAGMENT_MANAGER)
        .build();
    dialog.show(this, ERROR_LOADING_DIALOG_TAG);
  }

  @Override
  public void onAlertDialogPositiveClick(int requestCode, int which)
  {
    setAction(TRY_AGAIN);
    onTryAgainClicked();
  }

  @Override
  public void onAlertDialogNegativeClick(int requestCode, int which)
  {
    // no op
  }

  @Override
  public void onAlertDialogCancel(int requestCode)
  {
    setAction(PAUSE);
  }

  private static native int nativeGetBytesToDownload();
  private static native int nativeStartNextFileDownload(Listener listener);
  private static native void nativeCancelCurrentFile();
}
