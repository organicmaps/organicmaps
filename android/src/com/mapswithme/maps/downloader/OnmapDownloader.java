package com.mapswithme.maps.downloader;

import android.text.TextUtils;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;

import com.mapswithme.maps.MwmActivity;
import com.mapswithme.maps.R;
import com.mapswithme.maps.widget.WheelProgressView;
import com.mapswithme.util.Config;
import com.mapswithme.util.ConnectionState;
import com.mapswithme.util.StringUtils;
import com.mapswithme.util.UiUtils;

public class OnmapDownloader implements MwmActivity.LeftAnimationTrackListener
{
  private final View mFrame;
  private final TextView mParent;
  private final TextView mTitle;
  private final TextView mSize;
  private final WheelProgressView mProgress;
  private final Button mButton;

  private int mStorageSubscriptionSlot;

  private CountryItem mCurrentCountry;

  private final MapManager.StorageCallback mStorageCallback = new MapManager.StorageCallback()
  {
    @Override
    public void onStatusChanged(String countryId, int newStatus)
    {
      if (mCurrentCountry != null && mCurrentCountry.id.equals(countryId))
      {
        mCurrentCountry.update();
        updateState();
      }
    }

    @Override
    public void onProgress(String countryId, long localSize, long remoteSize)
    {
      if (mCurrentCountry != null && mCurrentCountry.id.equals(countryId))
      {
        mCurrentCountry.update();
        updateState();
      }
    }
  };

  private final MapManager.CurrentCountryChangedListener mCountryChangedListener = new MapManager.CurrentCountryChangedListener()
  {
    @Override
    public void onCurrentCountryChanged(String countryId)
    {
      mCurrentCountry = (TextUtils.isEmpty(countryId) ? null : CountryItem.fill(countryId));
      updateState();
    }
  };

  private void updateState()
  {
    boolean showFrame = (mCurrentCountry != null);
    if (showFrame)
    {
      boolean showProgress = (mCurrentCountry.status == CountryItem.STATUS_ENQUEUED ||
                              mCurrentCountry.status == CountryItem.STATUS_PROGRESS);

      showFrame = (showProgress ||
                   mCurrentCountry.status == CountryItem.STATUS_DOWNLOADABLE ||
                   mCurrentCountry.status == CountryItem.STATUS_FAILED);

      if (showFrame)
      {
        UiUtils.showIf(showProgress, mProgress);
        UiUtils.showIf(!showProgress, mButton);
        UiUtils.showIf(!TextUtils.isEmpty(mCurrentCountry.parentName), mParent);

        if (!TextUtils.isEmpty(mCurrentCountry.parentName))
          mParent.setText(mCurrentCountry.parentName);

        mTitle.setText(mCurrentCountry.name);
        mSize.setText(StringUtils.getFileSizeString(mCurrentCountry.totalSize));

        if (showProgress)
          mProgress.setProgress((int)(mCurrentCountry.progress * 100 / mCurrentCountry.totalSize));
        else
        {
          boolean failed = (mCurrentCountry.status == CountryItem.STATUS_FAILED);
          if (!failed &&
              !MapManager.nativeIsLegacyMode() &&
              Config.isAutodownloadMaps() &&
              ConnectionState.isWifiConnected())
            MapManager.nativeDownload(mCurrentCountry.id);

          mButton.setText(failed ? R.string.downloader_retry
                                 : R.string.download);
        }
      }
    }

    UiUtils.showIf(showFrame, mFrame);
  }

  public OnmapDownloader(MwmActivity activity)
  {
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
      }
    });

    mButton.setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        if (MapManager.nativeIsLegacyMode())
        {
          // TODO
          return;
        }

        if (mCurrentCountry.status == CountryItem.STATUS_FAILED)
          MapManager.nativeRetry(mCurrentCountry.id);
        else
          MapManager.nativeDownload(mCurrentCountry.id);
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
