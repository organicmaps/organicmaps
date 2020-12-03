package com.mapswithme.maps.downloader;

import android.location.Location;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MwmActivity;
import com.mapswithme.maps.R;
import com.mapswithme.maps.background.Notifier;
import com.mapswithme.maps.bookmarks.BookmarkCategoriesActivity;
import com.mapswithme.maps.bookmarks.BookmarksCatalogActivity;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.routing.RoutingController;
import com.mapswithme.maps.widget.WheelProgressView;
import com.mapswithme.util.Config;
import com.mapswithme.util.ConnectionState;
import com.mapswithme.util.StringUtils;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.statistics.Statistics;

import java.util.List;
import java.util.Locale;

public class OnmapDownloader implements MwmActivity.LeftAnimationTrackListener
{
  private static boolean sAutodownloadLocked;

  private final MwmActivity mActivity;
  private final View mFrame;
  private final TextView mParent;
  private final TextView mTitle;
  private final TextView mSize;
  private final WheelProgressView mProgress;
  private final Button mButton;

  @NonNull
  private final View mPromoContentDivider;
  @NonNull
  private final ViewGroup mBannerFrame;

  private int mStorageSubscriptionSlot;

  @Nullable
  private CountryItem mCurrentCountry;

  @Nullable
  private DownloaderPromoBanner mPromoBanner;

  private final MapManager.StorageCallback mStorageCallback = new MapManager.StorageCallback()
  {
    @Override
    public void onStatusChanged(List<MapManager.StorageCallbackData> data)
    {
      if (mCurrentCountry == null)
        return;

      for (MapManager.StorageCallbackData item : data)
      {
        if (!item.isLeafNode)
          continue;

        if (item.newStatus == CountryItem.STATUS_FAILED)
          MapManager.showError(mActivity, item, null);

        if (mCurrentCountry.id.equals(item.countryId))
        {
          mCurrentCountry.update();
          updateProgressState(false);

          return;
        }
      }
    }

    @Override
    public void onProgress(String countryId, long localSize, long remoteSize)
    {
      if (mCurrentCountry != null && mCurrentCountry.id.equals(countryId))
      {
        mCurrentCountry.update();
        updateProgressState(false);
      }
    }
  };

  private final MapManager.CurrentCountryChangedListener mCountryChangedListener = new MapManager.CurrentCountryChangedListener()
  {
    @Override
    public void onCurrentCountryChanged(String countryId)
    {
      mCurrentCountry = (TextUtils.isEmpty(countryId) ? null : CountryItem.fill(countryId));
      updateState(true);
    }
  };

  public void updateState(boolean shouldAutoDownload)
  {
    updateStateInternal(shouldAutoDownload);
  }

  private static boolean isMapDownloading(@Nullable CountryItem country)
  {
    if (country == null) return false;

    boolean enqueued = country.status == CountryItem.STATUS_ENQUEUED;
    boolean progress = country.status == CountryItem.STATUS_PROGRESS;
    boolean applying = country.status == CountryItem.STATUS_APPLYING;
    return enqueued || progress || applying;
  }

  private void updateProgressState(boolean shouldAutoDownload)
  {
    updateStateInternal(shouldAutoDownload);
  }

  private void updateStateInternal(boolean shouldAutoDownload)
  {
    boolean showFrame = (mCurrentCountry != null &&
                         !mCurrentCountry.present &&
                         !RoutingController.get().isNavigating());
    if (showFrame)
    {
      boolean enqueued = (mCurrentCountry.status == CountryItem.STATUS_ENQUEUED);
      boolean progress = (mCurrentCountry.status == CountryItem.STATUS_PROGRESS ||
                          mCurrentCountry.status == CountryItem.STATUS_APPLYING);
      boolean failed = (mCurrentCountry.status == CountryItem.STATUS_FAILED);

      showFrame = (enqueued || progress || failed ||
                   mCurrentCountry.status == CountryItem.STATUS_DOWNLOADABLE);

      if (showFrame)
      {
        boolean hasParent = !CountryItem.isRoot(mCurrentCountry.topmostParentId);

        UiUtils.showIf(progress || enqueued, mProgress);
        UiUtils.showIf(!progress && !enqueued, mButton);
        UiUtils.showIf(hasParent, mParent);

        if (hasParent)
          mParent.setText(mCurrentCountry.topmostParentName);

        mTitle.setText(mCurrentCountry.name);

        String sizeText;

        if (progress)
        {
          mProgress.setPending(false);
          mProgress.setProgress(mCurrentCountry.progress);
          sizeText = String.format(Locale.US, "%1$s %2$d%%", mActivity.getString(R.string.downloader_downloading), mCurrentCountry.progress);
        }
        else
        {
          if (enqueued)
          {
            sizeText = mActivity.getString(R.string.downloader_queued);
            mProgress.setPending(true);
          }
          else
          {
            sizeText = StringUtils.getFileSizeString(mActivity.getApplicationContext(), mCurrentCountry.totalSize);

            if (shouldAutoDownload &&
                Config.isAutodownloadEnabled() &&
                !sAutodownloadLocked &&
                !failed &&
                ConnectionState.INSTANCE.isWifiConnected())
            {
              Location loc = LocationHelper.INSTANCE.getSavedLocation();
              if (loc != null)
              {
                String country = MapManager.nativeFindCountry(loc.getLatitude(), loc.getLongitude());
                if (TextUtils.equals(mCurrentCountry.id, country) &&
                    MapManager.nativeHasSpaceToDownloadCountry(country))
                {
                  MapManager.nativeDownload(mCurrentCountry.id);

                  Statistics.INSTANCE.trackEvent(Statistics.EventName.DOWNLOADER_ACTION,
                                                 Statistics.params().add(Statistics.EventParam.ACTION, "download")
                                                                    .add(Statistics.EventParam.FROM, "map")
                                                                    .add("is_auto", "true")
                                                                    .add("scenario", "download"));
                }
              }
            }

            mButton.setText(failed ? R.string.downloader_retry
                                   : R.string.download);
          }
        }

        mSize.setText(sizeText);
      }
    }

    UiUtils.showIf(showFrame, mFrame);
    updateBannerVisibility();
  }

  public OnmapDownloader(MwmActivity activity)
  {
    mActivity = activity;
    mFrame = activity.findViewById(R.id.onmap_downloader);
    mParent = (TextView)mFrame.findViewById(R.id.downloader_parent);
    mTitle = (TextView)mFrame.findViewById(R.id.downloader_title);
    mSize = (TextView)mFrame.findViewById(R.id.downloader_size);

    View controls = mFrame.findViewById(R.id.downloader_controls_frame);
    mProgress = (WheelProgressView) controls.findViewById(R.id.wheel_downloader_progress);
    mButton = (Button) controls.findViewById(R.id.downloader_button);

    mProgress.setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        if (mCurrentCountry == null)
          return;

        MapManager.nativeCancel(mCurrentCountry.id);
        Statistics.INSTANCE.trackEvent(Statistics.EventName.DOWNLOADER_CANCEL,
                                       Statistics.params().add(Statistics.EventParam.FROM, "map"));
        setAutodownloadLocked(true);
      }
    });
    final Notifier notifier = Notifier.from(activity.getApplication());
    mButton.setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        MapManager.warnOn3g(mActivity, mCurrentCountry == null ? null : mCurrentCountry.id, new Runnable()
        {
          @Override
          public void run()
          {
            if (mCurrentCountry == null)
              return;

            boolean retry = (mCurrentCountry.status == CountryItem.STATUS_FAILED);
            if (retry)
            {
              notifier.cancelNotification(Notifier.ID_DOWNLOAD_FAILED);
              MapManager.nativeRetry(mCurrentCountry.id);
            }
            else
              MapManager.nativeDownload(mCurrentCountry.id);

            Statistics.INSTANCE.trackEvent(Statistics.EventName.DOWNLOADER_ACTION,
                                           Statistics.params().add(Statistics.EventParam.ACTION, (retry ? "retry" : "download"))
                                                              .add(Statistics.EventParam.FROM, "map")
                                                              .add("is_auto", "false")
                                                              .add("scenario", "download"));
          }
        });
      }
     });

    mPromoContentDivider = mFrame.findViewById(R.id.onmap_downloader_divider);
    mBannerFrame = mFrame.findViewById(R.id.banner_frame);
  }

  private void updateBannerVisibility()
  {
    if (mCurrentCountry == null || TextUtils.isEmpty(mCurrentCountry.id))
      return;

    DownloaderPromoBanner promoBanner = Framework.nativeGetDownloaderPromoBanner(mCurrentCountry.id);

    if (!isMapDownloading(mCurrentCountry) || promoBanner == null)
    {
      mPromoBanner = null;
      UiUtils.hide(mPromoContentDivider, mBannerFrame);
      return;
    }

    // No need to do anything when banners are equal.
    if (mPromoBanner != null && mPromoBanner.getType() == promoBanner.getType())
      return;

    mPromoBanner = promoBanner;

    mBannerFrame.removeAllViewsInLayout();

    LayoutInflater inflater = LayoutInflater.from(mActivity);
    View root = inflater.inflate(mPromoBanner.getType().getLayoutId(), mBannerFrame, true);
    View button = root.findViewById(R.id.banner_button);

    mPromoBanner.getType().getViewConfigStrategy().configureView(root, R.id.icon, R.id.text,
                                                                 R.id.banner_button);
    button.setOnClickListener(new BannerCallToActionListener());

    UiUtils.show(mPromoContentDivider, mBannerFrame);

    Statistics.ParameterBuilder builder =
        Statistics.makeDownloaderBannerParamBuilder(mPromoBanner.getType().toStatisticValue(),
                                                    mCurrentCountry.id);

    Statistics.INSTANCE.trackEvent(Statistics.EventName.DOWNLOADER_BANNER_SHOW, builder);
  }

  @Override
  public void onTrackStarted(boolean collapsed) {}

  @Override
  public void onTrackFinished(boolean collapsed) {}

  @Override
  public void onTrackLeftAnimation(float offset)
  {
    ViewGroup.MarginLayoutParams lp = (ViewGroup.MarginLayoutParams)mFrame.getLayoutParams();
    lp.leftMargin = (int)offset;
    mFrame.setLayoutParams(lp);
  }

  public void onPause()
  {
    if (mStorageSubscriptionSlot > 0)
    {
      MapManager.nativeUnsubscribe(mStorageSubscriptionSlot);
      mStorageSubscriptionSlot = 0;
      MapManager.nativeUnsubscribeOnCountryChanged();
    }
  }

  public void onResume()
  {
    if (mStorageSubscriptionSlot == 0)
    {
      mStorageSubscriptionSlot = MapManager.nativeSubscribe(mStorageCallback);
      MapManager.nativeSubscribeOnCountryChanged(mCountryChangedListener);
    }
  }

  public static void setAutodownloadLocked(boolean locked)
  {
    sAutodownloadLocked = locked;
  }

  private class BannerCallToActionListener implements View.OnClickListener
  {
    @Override
    public void onClick(View v)
    {
      if (mPromoBanner == null)
        return;

      mPromoBanner.getType().onAction(mActivity, mPromoBanner.getUrl());

      if (mCurrentCountry == null)
        return;

      Statistics.ParameterBuilder builder =
          Statistics.makeDownloaderBannerParamBuilder(mPromoBanner.getType().toStatisticValue(),
                                                      mCurrentCountry.id);
      Statistics.INSTANCE.trackEvent(Statistics.EventName.DOWNLOADER_BANNER_CLICK, builder);
    }
  }
}
