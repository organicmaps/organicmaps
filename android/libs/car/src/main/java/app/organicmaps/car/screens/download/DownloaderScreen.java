package app.organicmaps.car.screens.download;

import androidx.annotation.NonNull;
import androidx.car.app.CarContext;
import androidx.car.app.model.Action;
import androidx.car.app.model.Header;
import androidx.car.app.model.Pane;
import androidx.car.app.model.PaneTemplate;
import androidx.car.app.model.Row;
import androidx.car.app.model.Template;
import androidx.lifecycle.LifecycleOwner;
import app.organicmaps.car.R;
import app.organicmaps.car.screens.ErrorScreen;
import app.organicmaps.downloader.ErrorCodeHelper;
import app.organicmaps.sdk.OrganicMaps;
import app.organicmaps.sdk.car.screens.BaseScreen;
import app.organicmaps.sdk.downloader.CountryItem;
import app.organicmaps.sdk.downloader.MapManager;
import app.organicmaps.sdk.util.StringUtils;
import app.organicmaps.sdk.util.concurrency.UiThread;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

class DownloaderScreen extends BaseScreen
{
  private static final long PROGRESS_REFRESH_INTERVAL_MS = 1000;

  @NonNull
  private final Map<String, CountryItem> mMissingMaps;
  private final long mTotalSize;
  private final boolean mIsCancelActionDisabled;

  private long mDownloadedMapsSize = 0;
  private int mSubscriptionSlot = 0;
  private boolean mIsDownloadFailed = false;
  private boolean mIsProgressRefreshScheduled = false;

  @NonNull
  private final Runnable mProgressRefresh;

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
      final CountryItem item = mMissingMaps.get(countryId);
      if (item != null)
      {
        item.downloadedBytes = localSize;
        scheduleProgressRefresh();
      }
    }
  };

  DownloaderScreen(@NonNull final CarContext carContext, @NonNull OrganicMaps organicMapsContext,
                   @NonNull final List<CountryItem> missingMaps, final boolean isCancelActionDisabled)
  {
    super(carContext, organicMapsContext);
    setMarker(DownloadMapsScreen.MARKER);
    setResult(false);

    MapManager.nativeEnableDownloadOn3g();

    mMissingMaps = new HashMap<>();
    for (final CountryItem item : missingMaps)
      mMissingMaps.put(item.id, item);
    mProgressRefresh = () ->
    {
      mIsProgressRefreshScheduled = false;
      if (!mMissingMaps.isEmpty())
        invalidate();
    };
    mTotalSize = DownloaderHelpers.getMapsSize(mMissingMaps.values());
    mIsCancelActionDisabled = isCancelActionDisabled;
  }

  @Override
  public void onResume(@NonNull LifecycleOwner owner)
  {
    super.onResume(owner);
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
    super.onPause(owner);
    UiThread.cancelDelayedTasks(mProgressRefresh);
    mIsProgressRefreshScheduled = false;
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
  protected Template onGetTemplateImpl()
  {
    final Pane pane = new Pane.Builder().addRow(createProgressRow()).build();
    final PaneTemplate.Builder builder = new PaneTemplate.Builder(pane);

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
  private Row createProgressRow()
  {
    final long downloadedSize = getDownloadedSize();
    final String progressPercent = StringUtils.formatPercent((double) downloadedSize / mTotalSize, true);
    final String totalSizeStr = StringUtils.getFileSizeString(getCarContext(), mTotalSize);
    final String downloadedSizeStr = StringUtils.getFileSizeString(getCarContext(), downloadedSize);

    // Keep progress in secondary text: changing the template title or row titles would consume the host's quota.
    return new Row.Builder()
        .setTitle(getCarContext().getString(R.string.downloader_loading_ios))
        .addText(progressPercent)
        .addText(downloadedSizeStr + " / " + totalSizeStr)
        .build();
  }

  private long getDownloadedSize()
  {
    long downloadedSize = 0;

    for (final CountryItem map : mMissingMaps.values())
      downloadedSize += map.downloadedBytes;

    return downloadedSize + mDownloadedMapsSize;
  }

  private void scheduleProgressRefresh()
  {
    if (mIsProgressRefreshScheduled)
      return;

    mIsProgressRefreshScheduled = true;
    UiThread.runLater(mProgressRefresh, PROGRESS_REFRESH_INTERVAL_MS);
  }

  private void onError(@NonNull final MapManager.StorageCallbackData data)
  {
    mIsDownloadFailed = true;
    final ErrorScreen.Builder builder = new ErrorScreen.Builder(getCarContext(), getOrganicMapsContext())
                                            .setTitle(R.string.country_status_download_failed)
                                            .setErrorMessage(ErrorCodeHelper.getErrorCodeStrRes(data.errorCode))
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
