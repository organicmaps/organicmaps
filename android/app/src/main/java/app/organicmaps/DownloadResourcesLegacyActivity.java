package app.organicmaps;

import android.annotation.SuppressLint;
import android.app.Dialog;
import android.content.ComponentName;
import android.content.Intent;
import android.location.Location;
import android.os.Bundle;
import android.text.TextUtils;
import android.view.View;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.TextView;

import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.annotation.CallSuper;
import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StringRes;
import androidx.annotation.StyleRes;
import androidx.core.view.ViewCompat;
import app.organicmaps.base.BaseMwmFragmentActivity;
import app.organicmaps.downloader.CountryItem;
import app.organicmaps.downloader.MapManager;
import app.organicmaps.intent.Factory;
import app.organicmaps.location.LocationHelper;
import app.organicmaps.location.LocationListener;
import app.organicmaps.util.Config;
import app.organicmaps.util.ConnectionState;
import app.organicmaps.util.StringUtils;
import app.organicmaps.util.UiUtils;
import app.organicmaps.util.Utils;
import app.organicmaps.util.WindowInsetUtils.PaddingInsetsListener;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;
import com.google.android.material.progressindicator.LinearProgressIndicator;

import java.util.List;
import java.util.Objects;

@SuppressLint("StringFormatMatches")
public class DownloadResourcesLegacyActivity extends BaseMwmFragmentActivity
{
  private static final String TAG = DownloadResourcesLegacyActivity.class.getSimpleName();

  // Error codes, should match the same codes in JNI
  private static final int ERR_DOWNLOAD_SUCCESS = 0;
  private static final int ERR_DISK_ERROR = -1;
  private static final int ERR_NOT_ENOUGH_FREE_SPACE = -2;
  private static final int ERR_STORAGE_DISCONNECTED = -3;
  private static final int ERR_DOWNLOAD_ERROR = -4;
  private static final int ERR_NO_MORE_FILES = -5;
  private static final int ERR_FILE_IN_PROGRESS = -6;

  private TextView mTvMessage;
  private LinearProgressIndicator mProgress;
  private Button mBtnDownload;
  private CheckBox mChbDownloadCountry;

  private String mCurrentCountry;

  @Nullable
  private Dialog mAlertDialog;

  @NonNull
  private ActivityResultLauncher<Intent> mApiRequest;

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

  private interface Listener
  {
    // Called by JNI.
    @Keep
    @SuppressWarnings("unused")
    void onProgress(int percent);

    // Called by JNI.
    @Keep
    @SuppressWarnings("unused")
    void onFinish(int errorCode);
  }

  private final LocationListener mLocationListener = new LocationListener()
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

      LocationHelper.from(DownloadResourcesLegacyActivity.this).removeListener(this);
    }
  };

  private final Listener mResourcesDownloadListener = new Listener()
  {
    @Override
    public void onProgress(final int percent)
    {
      if (!isFinishing())
        mProgress.setProgressCompat(percent, true);
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
      mProgress.setProgressCompat((int) localSize, true);
    }
  };

  @CallSuper
  @Override
  protected void onSafeCreate(@Nullable Bundle savedInstanceState)
  {
    super.onSafeCreate(savedInstanceState);
    UiUtils.setLightStatusBar(this, true);
    setContentView(R.layout.activity_download_resources);
    final View view = getWindow().getDecorView().findViewById(android.R.id.content);
    ViewCompat.setOnApplyWindowInsetsListener(view, PaddingInsetsListener.allSides());
    initViewsAndListeners();
    mApiRequest = registerForActivityResult(new ActivityResultContracts.StartActivityForResult(), result -> {
      setResult(result.getResultCode(), result.getData());
      finish();
    });

    if (prepareFilesDownload(false))
    {
      Utils.keepScreenOn(true, getWindow());

      setAction(DOWNLOAD);

      return;
    }

    showMap();
  }

  @CallSuper
  @Override
  protected void onSafeDestroy()
  {
    super.onSafeDestroy();
    mApiRequest.unregister();
    mApiRequest = null;
    Utils.keepScreenOn(Config.isKeepScreenOnEnabled(), getWindow());
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
      LocationHelper.from(this).addListener(mLocationListener);
  }

  @Override
  protected void onPause()
  {
    super.onPause();
    LocationHelper.from(this).removeListener(mLocationListener);
    if (mAlertDialog != null && mAlertDialog.isShowing())
      mAlertDialog.dismiss();
    mAlertDialog = null;
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
      mProgress.setProgressCompat(0, true);
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
    mBtnNames[RESUME] = getString(R.string.continue_button);

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

  public void showMap()
  {
    if (!mAreResourcesDownloaded)
      return;

    // Re-use original intent to retain all flags and payload.
    // https://github.com/organicmaps/organicmaps/issues/6944
    final Intent intent = Objects.requireNonNull(getIntent());
    intent.setComponent(new ComponentName(this, MwmActivity.class));

    // Disable animation because MwmActivity should appear exactly over this one
    intent.addFlags(Intent.FLAG_ACTIVITY_NO_ANIMATION | Intent.FLAG_ACTIVITY_CLEAR_TOP);

    // See {@link SplashActivity.processNavigation()}
    if (Factory.isStartedForApiResult(intent))
    {
      // Wait for the result from MwmActivity for API callers.
      mApiRequest.launch(intent);
      return;
    }

    startActivity(intent);
    finish();
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
        mProgress.setProgressCompat(0, true);

        mCountryDownloadListenerSlot = MapManager.nativeSubscribe(mCountryDownloadListener);
        MapManager.startDownload(mCurrentCountry);
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
      showErrorDialog(result);
    }
  }

  private void showErrorDialog(int result)
  {
    if (mAlertDialog != null && mAlertDialog.isShowing())
      return;

    @StringRes final int titleId;
    @StringRes final int messageId = switch (result)
    {
      case ERR_NOT_ENOUGH_FREE_SPACE ->
      {
        titleId = R.string.routing_not_enough_space;
        yield R.string.not_enough_free_space_on_sdcard;
      }
      case ERR_STORAGE_DISCONNECTED ->
      {
        titleId = R.string.disconnect_usb_cable_title;
        yield R.string.disconnect_usb_cable;
      }
      case ERR_DOWNLOAD_ERROR ->
      {
        titleId = R.string.connection_failure;
        yield (ConnectionState.INSTANCE.isConnected() ? R.string.download_has_failed
                                                      : R.string.common_check_internet_connection_dialog);
      }
      case ERR_DISK_ERROR ->
      {
        titleId = R.string.disk_error_title;
        yield R.string.disk_error;
      }
      default -> throw new AssertionError("Unexpected result code = " + result);
    };

    mAlertDialog = new MaterialAlertDialogBuilder(this, R.style.MwmTheme_AlertDialog)
        .setTitle(titleId)
        .setMessage(messageId)
        .setCancelable(true)
        .setOnCancelListener((dialog) -> setAction(PAUSE))
        .setPositiveButton(R.string.try_again, (dialog, which) -> {
          setAction(TRY_AGAIN);
          onTryAgainClicked();
        })
        .setOnDismissListener(dialog -> mAlertDialog = null)
        .show();
  }

  @Override
  @StyleRes
  public int getThemeResourceId(@NonNull String theme)
  {
    return R.style.MwmTheme_DownloadResourcesLegacy;
  }

  private static native int nativeGetBytesToDownload();
  private static native int nativeStartNextFileDownload(Listener listener);
  private static native void nativeCancelCurrentFile();
}
