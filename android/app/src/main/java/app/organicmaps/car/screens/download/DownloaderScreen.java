package app.organicmaps.car.screens.download;

import android.text.TextUtils;
import androidx.annotation.NonNull;
import androidx.car.app.CarContext;
import androidx.car.app.constraints.ConstraintManager;
import androidx.car.app.model.Action;
import androidx.car.app.model.Header;
import androidx.car.app.model.MessageTemplate;
import androidx.car.app.model.Template;
import androidx.lifecycle.LifecycleOwner;
import app.organicmaps.R;
import app.organicmaps.car.screens.ErrorScreen;
import app.organicmaps.car.screens.base.BaseScreen;
import app.organicmaps.sdk.downloader.CountryItem;
import app.organicmaps.sdk.downloader.MapManager;
import app.organicmaps.sdk.util.StringUtils;
import app.organicmaps.sdk.util.concurrency.UiThread;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

class DownloaderScreen extends BaseScreen
{
  @NonNull
  private final Map<String, CountryItem> mMissingMaps;
  private final long mTotalSize;
  private final boolean mIsCancelActionDisabled;
  private final boolean mIsAppRefreshEnabled;

  private long mDownloadedMapsSize = 0;
  private int mSubscriptionSlot = 0;
  private boolean mIsDownloadFailed = false;

  @NonNull
  private final MapManager.StorageCallback mStorageCallback = new MapManager.StorageCallback() {
    @Override
    public void onStatusChanged(@NonNull final List<MapManager.StorageCallbackData> data)
    {
      for (final MapManager.StorageCallbackData item : data)
      {
        if (item.newStatus == CountryItem.STATUS_FAILED)
        {
          onError(item);
          return;
        }

        final CountryItem map = mMissingMaps.get(item.countryId);
        if (map == null)
          continue;

        map.update();
        if (map.present)
        {
          mDownloadedMapsSize += map.totalSize;
          mMissingMaps.remove(map.id);
        }
      }

      if (mMissingMaps.isEmpty())
      {
        setResult(true);
        UiThread.runLater(DownloaderScreen.this::finish);
      }
      else
        invalidate();
    }

    @Override
    public void onProgress(String countryId, long localSize, long remoteSize)
    {
      if (!mIsAppRefreshEnabled || TextUtils.isEmpty(countryId))
        return;

      final CountryItem item = mMissingMaps.get(countryId);
      if (item != null)
      {
        item.update();
        invalidate();
      }
    }
  };

  DownloaderScreen(@NonNull final CarContext carContext, @NonNull final List<CountryItem> missingMaps,
                   final boolean isCancelActionDisabled)
  {
    super(carContext);
    setMarker(DownloadMapsScreen.MARKER);
    setResult(false);

    MapManager.nativeEnableDownloadOn3g();

    mMissingMaps = new HashMap<>();
    for (final CountryItem item : missingMaps)
      mMissingMaps.put(item.id, item);
    mTotalSize = DownloaderHelpers.getMapsSize(mMissingMaps.values());
    mIsCancelActionDisabled = isCancelActionDisabled;
    mIsAppRefreshEnabled = carContext.getCarService(ConstraintManager.class).isAppDrivenRefreshEnabled();
  }

  @Override
  public void onResume(@NonNull LifecycleOwner owner)
  {
    if (mSubscriptionSlot == 0)
      mSubscriptionSlot = MapManager.nativeSubscribe(mStorageCallback);
    for (final var item : mMissingMaps.entrySet())
    {
      item.getValue().update();
      MapManager.startDownload(item.getKey());
    }
  }

  @Override
  public void onPause(@NonNull LifecycleOwner owner)
  {
    if (!mIsDownloadFailed)
      cancelMapsDownloading();
    if (mSubscriptionSlot != 0)
    {
      MapManager.nativeUnsubscribe(mSubscriptionSlot);
      mSubscriptionSlot = 0;
    }
  }

  @NonNull
  @Override
  public Template onGetTemplate()
  {
    final MessageTemplate.Builder builder = new MessageTemplate.Builder(getText());
    builder.setLoading(true);

    final Header.Builder headerBuilder = new Header.Builder();
    if (mIsCancelActionDisabled)
      headerBuilder.setStartHeaderAction(Action.APP_ICON);
    else
      headerBuilder.setStartHeaderAction(Action.BACK);
    headerBuilder.setTitle(getCarContext().getString(R.string.notification_channel_downloader));
    builder.setHeader(headerBuilder.build());

    return builder.build();
  }

  @NonNull
  private String getText()
  {
    if (!mIsAppRefreshEnabled)
      return getCarContext().getString(R.string.downloader_loading_ios);

    final long downloadedSize = getDownloadedSize();
    final String progressPercent = StringUtils.formatPercent((double) downloadedSize / mTotalSize);
    final String totalSizeStr = StringUtils.getFileSizeString(getCarContext(), mTotalSize);
    final String downloadedSizeStr = StringUtils.getFileSizeString(getCarContext(), downloadedSize);

    return progressPercent + "\n" + downloadedSizeStr + " / " + totalSizeStr;
  }

  private long getDownloadedSize()
  {
    long downloadedSize = 0;

    for (final CountryItem map : mMissingMaps.values())
      downloadedSize += map.downloadedBytes;

    return downloadedSize + mDownloadedMapsSize;
  }

  private void onError(@NonNull final MapManager.StorageCallbackData data)
  {
    mIsDownloadFailed = true;
    final ErrorScreen.Builder builder = new ErrorScreen.Builder(getCarContext())
                                            .setTitle(R.string.country_status_download_failed)
                                            .setErrorMessage(MapManager.getErrorCodeStrRes(data.errorCode))
                                            .setPositiveButton(R.string.downloader_retry, null);
    if (!mIsCancelActionDisabled)
      builder.setNegativeButton(R.string.cancel, this::finish);
    getScreenManager().push(builder.build());
  }

  private void cancelMapsDownloading()
  {
    for (final String map : mMissingMaps.keySet())
      MapManager.nativeCancel(map);
  }
}
