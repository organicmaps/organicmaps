package app.organicmaps.car.screens.download;

import androidx.annotation.NonNull;
import androidx.car.app.CarContext;
import androidx.car.app.constraints.ConstraintManager;
import androidx.car.app.model.Action;
import androidx.car.app.model.Header;
import androidx.car.app.model.MessageTemplate;
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
import java.util.HashSet;
import java.util.List;
import java.util.Set;

class DownloaderScreen extends BaseScreen
{
  @NonNull
  private final Set<String> mMissingMaps;
  @NonNull
  private final String[] mMaps;
  private final boolean mIsCancelActionDisabled;
  private final boolean mIsAppRefreshEnabled;

  private int mSubscriptionSlot = 0;
  private boolean mIsDownloadFailed = false;

  @NonNull
  private final MapManager.StorageCallback mStorageCallback = new MapManager.StorageCallback() {
    @Override
    public void onStatusChanged(@NonNull final List<MapManager.StorageCallbackData> data)
    {
      for (final MapManager.StorageCallbackData item : data)
      {
        if (!mMissingMaps.contains(item.countryId))
          continue;

        if (item.newStatus == CountryItem.STATUS_FAILED)
        {
          onError(item);
          return;
        }

        if (item.newStatus == CountryItem.STATUS_DONE)
          mMissingMaps.remove(item.countryId);
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
      if (!mIsAppRefreshEnabled)
        return;

      if (mMissingMaps.contains(countryId))
        invalidate();
    }
  };

  DownloaderScreen(@NonNull final CarContext carContext, @NonNull OrganicMaps organicMapsContext,
                   @NonNull final List<CountryItem> missingMaps, final boolean isCancelActionDisabled)
  {
    super(carContext, organicMapsContext);
    setMarker(DownloadMapsScreen.MARKER);
    setResult(false);

    MapManager.nativeEnableDownloadOn3g();

    mMissingMaps = new HashSet<>();
    for (final CountryItem item : missingMaps)
      mMissingMaps.add(item.id);
    mMaps = mMissingMaps.toArray(new String[0]);
    mIsCancelActionDisabled = isCancelActionDisabled;
    mIsAppRefreshEnabled = carContext.getCarService(ConstraintManager.class).isAppDrivenRefreshEnabled();
  }

  @Override
  public void onResume(@NonNull LifecycleOwner owner)
  {
    super.onResume(owner);
    if (mSubscriptionSlot == 0)
      mSubscriptionSlot = MapManager.nativeSubscribe(mStorageCallback);
    MapManager.startDownload(mMissingMaps.toArray(new String[0]));
  }

  @Override
  public void onPause(@NonNull LifecycleOwner owner)
  {
    super.onPause(owner);
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

    final long[] progress = MapManager.nativeGetOverallProgressBytes(mMaps);
    final double fraction = progress[1] == 0 ? 0 : (double) progress[0] / progress[1];
    final String progressPercent = StringUtils.formatPercent(fraction, true);
    final String totalSizeStr = StringUtils.getFileSizeString(getCarContext(), progress[1]);
    final String downloadedSizeStr = StringUtils.getFileSizeString(getCarContext(), progress[0]);

    return progressPercent + "\n" + downloadedSizeStr + " / " + totalSizeStr;
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
    for (final String map : mMissingMaps)
      MapManager.nativeCancel(map);
  }
}
