package com.mapswithme.maps.downloader;

import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import java.util.Locale;

import com.mapswithme.maps.R;
import com.mapswithme.util.StringUtils;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.statistics.Statistics;

class BottomPanel
{
  private final DownloaderFragment mFragment;
  private final View mFrame;
  private final TextView mText;
  private final Button mButton;

  private final View.OnClickListener mDownloadListener = new View.OnClickListener()
  {
    @Override
    public void onClick(View v)
    {
      MapManager.nativeDownload(mFragment.getAdapter().getCurrentParent());

      Statistics.INSTANCE.trackEvent(Statistics.EventName.DOWNLOADER_ACTION,
                                     Statistics.params().add(Statistics.EventParam.ACTION, "download")
                                                        .add(Statistics.EventParam.FROM, "downloader")
                                                        .add("is_auto", "false")
                                                        .add("scenario", "download_group"));
    }
  };

  private final View.OnClickListener mUpdateListener = new View.OnClickListener()
  {
    @Override
    public void onClick(View v)
    {
      MapManager.nativeUpdate(mFragment.getAdapter().getCurrentParent());

      Statistics.INSTANCE.trackEvent(Statistics.EventName.DOWNLOADER_ACTION,
                                     Statistics.params().add(Statistics.EventParam.ACTION, "update")
                                                        .add(Statistics.EventParam.FROM, "downloader")
                                                        .add("is_auto", "false")
                                                        .add("scenario", "update_all"));
    }
  };

  private final View.OnClickListener mCancelListener = new View.OnClickListener()
  {
    @Override
    public void onClick(View v)
    {
        MapManager.nativeCancel(mFragment.getAdapter().getCurrentParent());

        Statistics.INSTANCE.trackEvent(Statistics.EventName.DOWNLOADER_CANCEL,
                                       Statistics.params().add(Statistics.EventParam.FROM, "downloader"));
    }
  };

  BottomPanel(DownloaderFragment fragment, View frame)
  {
    mFragment = fragment;
    mFrame = frame;
    mText = (TextView) mFrame.findViewById(R.id.tv__text);
    mButton = (Button) mFrame.findViewById(R.id.btn__action);
  }

  private void setUpdateAllState(UpdateInfo info)
  {
    UiUtils.setTextAndShow(mText, String.format(Locale.US, "%s: %d (%s)", mFragment.getString(R.string.downloader_status_maps),
                                                                          info.filesCount,
                                                                          StringUtils.getFileSizeString(info.totalSize)));
    mButton.setText(R.string.downloader_update_all_button);
    mButton.setOnClickListener(mUpdateListener);
  }

  private void setDownloadAllState()
  {
    UiUtils.invisible(mText);
    mButton.setText(R.string.downloader_download_all_button);
    mButton.setOnClickListener(mDownloadListener);
  }

  private void setCancelState()
  {
    UiUtils.invisible(mText);
    mButton.setText(R.string.downloader_cancel_all);
    mButton.setOnClickListener(mCancelListener);
  }

  public void update()
  {
    DownloaderAdapter adapter = mFragment.getAdapter();
    boolean search = adapter.isSearchResultsMode();

    boolean show = !search;
    if (show)
    {
      String root = adapter.getCurrentParent();

      if (root == null)
      {
        if (MapManager.nativeIsDownloading())
          setCancelState();
        else
        {
          UpdateInfo info = MapManager.nativeGetUpdateInfo(null);
          show = (info != null && info.filesCount > 0);
          if (show)
            setUpdateAllState(info);
        }
      }
      else
      {
        int status = MapManager.nativeGetStatus(root);
        switch (status)
        {
        case CountryItem.STATUS_DONE:
          show = false;
          break;

        case CountryItem.STATUS_PROGRESS:
        case CountryItem.STATUS_ENQUEUED:
          setCancelState();
          break;

        default:
          setDownloadAllState();
        }
      }
    }

    UiUtils.showIf(show, mFrame);
  }
}
