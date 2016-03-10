package com.mapswithme.maps.downloader;

import android.content.DialogInterface;
import android.support.v7.app.AlertDialog;
import android.text.TextUtils;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;

import java.util.ArrayDeque;
import java.util.List;
import java.util.Queue;

import com.mapswithme.maps.MwmActivity;
import com.mapswithme.maps.R;
import com.mapswithme.maps.background.Notifier;
import com.mapswithme.maps.routing.RoutingController;
import com.mapswithme.maps.widget.WheelProgressView;
import com.mapswithme.util.Config;
import com.mapswithme.util.ConnectionState;
import com.mapswithme.util.StringUtils;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.statistics.Statistics;

public class OnmapDownloader implements MwmActivity.LeftAnimationTrackListener
{
  private static final int CANCEL_LOCK_TIMEOUT_MS = 30 * 1000;
  private static final int CANCEL_LOCK_TIMES = 3;

  private final MwmActivity mActivity;
  private final View mFrame;
  private final TextView mParent;
  private final TextView mTitle;
  private final TextView mSize;
  private final WheelProgressView mProgress;
  private final Button mButton;

  private int mStorageSubscriptionSlot;

  private CountryItem mCurrentCountry;

  private final Queue<Long> mCancelTimestamps = new ArrayDeque<>();

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
          MapManager.showError(mActivity, item);

        if (mCurrentCountry.id.equals(item.countryId))
        {
          mCurrentCountry.update();
          updateState(false);

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
        updateState(false);
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
    boolean showFrame = (mCurrentCountry != null &&
                         !RoutingController.get().isNavigating());
    if (showFrame)
    {
      boolean enqueued = (mCurrentCountry.status == CountryItem.STATUS_ENQUEUED);
      boolean progress = (mCurrentCountry.status == CountryItem.STATUS_PROGRESS);
      boolean failed = (mCurrentCountry.status == CountryItem.STATUS_FAILED);

      showFrame = (enqueued || progress || failed ||
                   mCurrentCountry.status == CountryItem.STATUS_DOWNLOADABLE);

      if (showFrame)
      {
        boolean hasParent = !TextUtils.isEmpty(mCurrentCountry.parentName);

        UiUtils.showIf(progress || enqueued, mProgress);
        UiUtils.showIf(!progress && !enqueued, mButton);
        UiUtils.showIf(hasParent, mParent);

        if (hasParent)
          mParent.setText(mCurrentCountry.parentName);

        mTitle.setText(mCurrentCountry.name);

        if (progress)
        {
          mSize.setText(StringUtils.getFileSizeString(mCurrentCountry.totalSize));
          mProgress.setProgress((int) (mCurrentCountry.progress * 100L / mCurrentCountry.totalSize));
        }
        else
        {
          if (enqueued)
          {
            mSize.setText(R.string.downloader_queued);
            mProgress.setProgress(0);
          }
          else
          {
            if (shouldAutoDownload &&
                !failed &&
                !MapManager.nativeIsLegacyMode() &&
                Config.isAutodownloadMaps() &&
                ConnectionState.isWifiConnected())
            {
              MapManager.nativeDownload(mCurrentCountry.id);

              Statistics.INSTANCE.trackEvent(Statistics.EventName.DOWNLOADER_ACTION,
                                             Statistics.params().add(Statistics.EventParam.ACTION, "download")
                                                       .add(Statistics.EventParam.FROM, "map")
                                                       .add("is_auto", "true")
                                                       .add("scenario", "download"));
            }

            mButton.setText(failed ? R.string.downloader_retry
                                   : R.string.download);
          }
        }
      }
    }

    UiUtils.showIf(showFrame, mFrame);
  }

  private void offerDisableAutodownloading()
  {
    Config.setAutodownloadDisableOfferShown();

    new AlertDialog.Builder(mActivity)
        .setTitle(R.string.autodownload)
        .setMessage(R.string.downloader_offer_disable_autodownload)
        .setNegativeButton(android.R.string.no, null)
        .setPositiveButton(android.R.string.yes, new DialogInterface.OnClickListener()
        {
          @Override
          public void onClick(DialogInterface dialog, int which)
          {
            Config.setAutodownloadMaps(false);
          }
        }).show();
  }

  public OnmapDownloader(MwmActivity activity)
  {
    mActivity = activity;
    mFrame = activity.findViewById(R.id.onmap_downloader);
    mParent = (TextView)mFrame.findViewById(R.id.downloader_parent);
    mTitle = (TextView)mFrame.findViewById(R.id.downloader_title);
    mSize = (TextView)mFrame.findViewById(R.id.downloader_size);

    View controls = mFrame.findViewById(R.id.downloader_controls_frame);
    mProgress = (WheelProgressView) controls.findViewById(R.id.downloader_progress);
    mButton = (Button) controls.findViewById(R.id.downloader_button);

    mProgress.setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        MapManager.nativeCancel(mCurrentCountry.id);
        Statistics.INSTANCE.trackEvent(Statistics.EventName.DOWNLOADER_CANCEL,
                                       Statistics.params().add(Statistics.EventParam.FROM, "map"));

        if (!Config.isAutodownloadMaps() || Config.isAutodownloadDisableOfferShown())
          return;

        long now = System.currentTimeMillis();
        if (mCancelTimestamps.size() + 1 == CANCEL_LOCK_TIMES)
        {
          boolean showDialog = true;

          // Clean-up outdated events
          do
          {
            long ts = mCancelTimestamps.peek();
            if (ts + CANCEL_LOCK_TIMEOUT_MS > now)
              break;

            mCancelTimestamps.poll();
            showDialog = false;
          } while (!mCancelTimestamps.isEmpty());

          if (showDialog)
          {
            mCancelTimestamps.clear();
            offerDisableAutodownloading();
            return;
          }
        }

        mCancelTimestamps.add(now);
      }
    });

    mButton.setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        if (MapManager.nativeIsLegacyMode())
        {
          mActivity.showDownloader(false);
          return;
        }

        boolean retry = (mCurrentCountry.status == CountryItem.STATUS_FAILED);
        if (retry)
        {
          Notifier.cancelDownloadFailed();
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

    UiUtils.updateAccentButton(mButton);
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
    }

    MapManager.nativeUnsubscribeOnCountryChanged();
  }

  public void onResume()
  {
    mStorageSubscriptionSlot = MapManager.nativeSubscribe(mStorageCallback);
    MapManager.nativeSubscribeOnCountryChanged(mCountryChangedListener);
  }
}
