package com.mapswithme.maps.downloader;

import android.support.design.widget.FloatingActionButton;
import android.view.View;
import android.widget.Button;

import java.util.Locale;

import com.mapswithme.maps.R;
import com.mapswithme.util.StringUtils;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.statistics.Statistics;

class BottomPanel
{
  private final DownloaderFragment mFragment;
  private final FloatingActionButton mFab;
  private final Button mButton;

  private final View.OnClickListener mDownloadListener = new View.OnClickListener()
  {
    @Override
    public void onClick(View v)
    {
      MapManager.warn3gAndDownload(mFragment.getActivity(), mFragment.getCurrentRoot(), new Runnable()
      {
        @Override
        public void run()
        {
          Statistics.INSTANCE.trackEvent(Statistics.EventName.DOWNLOADER_ACTION,
                                         Statistics.params().add(Statistics.EventParam.ACTION, "download")
                                                            .add(Statistics.EventParam.FROM, "downloader")
                                                            .add("is_auto", "false")
                                                            .add("scenario", "download_group"));
        }
      });
    }
  };

  private final View.OnClickListener mUpdateListener = new View.OnClickListener()
  {
    @Override
    public void onClick(View v)
    {
      final String country = mFragment.getCurrentRoot();
      MapManager.warnOn3gUpdate(mFragment.getActivity(), country, new Runnable()
      {
        @Override
        public void run()
        {
          MapManager.nativeUpdate(country);

          Statistics.INSTANCE.trackEvent(Statistics.EventName.DOWNLOADER_ACTION,
                                         Statistics.params().add(Statistics.EventParam.ACTION, "update")
                                                            .add(Statistics.EventParam.FROM, "downloader")
                                                            .add("is_auto", "false")
                                                            .add("scenario", "update_all"));
        }
      });
    }
  };

  private final View.OnClickListener mCancelListener = new View.OnClickListener()
  {
    @Override
    public void onClick(View v)
    {
        MapManager.nativeCancel(mFragment.getCurrentRoot());

        Statistics.INSTANCE.trackEvent(Statistics.EventName.DOWNLOADER_CANCEL,
                                       Statistics.params().add(Statistics.EventParam.FROM, "downloader"));
    }
  };

  BottomPanel(DownloaderFragment fragment, View frame)
  {
    mFragment = fragment;

    mFab = (FloatingActionButton) frame.findViewById(R.id.fab);
    mFab.setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        if (mFragment.getAdapter() != null )
          mFragment.getAdapter().setAvailableMapsMode();
        update();
      }
    });

    mButton = (Button) frame.findViewById(R.id.action);

    UiUtils.updateAccentButton(mButton);
  }

  private void setUpdateAllState(UpdateInfo info)
  {
    mButton.setText(String.format(Locale.US, "%s (%s)", mFragment.getString(R.string.downloader_update_all_button),
                                                        StringUtils.getFileSizeString(info.totalSize)));
    mButton.setOnClickListener(mUpdateListener);
  }

  private void setDownloadAllState()
  {
    mButton.setText(R.string.downloader_download_all_button);
    mButton.setOnClickListener(mDownloadListener);
  }

  private void setCancelState()
  {
    mButton.setText(R.string.downloader_cancel_all);
    mButton.setOnClickListener(mCancelListener);
  }

  public void update()
  {
    DownloaderAdapter adapter = mFragment.getAdapter();
    boolean search = adapter != null && adapter.isSearchResultsMode();

    boolean show = !search;
    UiUtils.showIf(show && adapter != null && adapter.isMyMapsMode(), mFab);

    if (show)
    {
      String root = adapter != null ? adapter.getCurrentRootId() : "";
      if (adapter != null && adapter.isMyMapsMode())
      {
        int status = MapManager.nativeGetStatus(root);
        switch (status)
        {
        case CountryItem.STATUS_UPDATABLE:
          UpdateInfo info = MapManager.nativeGetUpdateInfo(root);
          setUpdateAllState(info);
          break;

        case CountryItem.STATUS_DOWNLOADABLE:  // Special case for "Countries" node when no maps currently downloaded.
        case CountryItem.STATUS_DONE:
        case CountryItem.STATUS_PARTLY:
          show = false;
          break;

        case CountryItem.STATUS_PROGRESS:
        case CountryItem.STATUS_APPLYING:
        case CountryItem.STATUS_ENQUEUED:
          setCancelState();
          break;

        case CountryItem.STATUS_FAILED:
          setDownloadAllState();
          break;

        default:
          throw new IllegalArgumentException("Inappropriate status for \"" + root + "\": " + status);
        }
      }
      else
      {
        show = !CountryItem.isRoot(root);
        if (show)
        {
          int status = MapManager.nativeGetStatus(root);
          switch (status)
          {
          case CountryItem.STATUS_UPDATABLE:
            UpdateInfo info = MapManager.nativeGetUpdateInfo(root);
            setUpdateAllState(info);
            break;

          case CountryItem.STATUS_DONE:
            show = false;
            break;

          case CountryItem.STATUS_PROGRESS:
          case CountryItem.STATUS_APPLYING:
          case CountryItem.STATUS_ENQUEUED:
            setCancelState();
            break;

          default:
            setDownloadAllState();
          }
        }
      }
    }

    UiUtils.showIf(show, mButton);
  }
}
