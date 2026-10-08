package app.organicmaps.downloader;

import static app.organicmaps.sdk.downloader.CountryItem.STATUS_APPLYING;
import static app.organicmaps.sdk.downloader.CountryItem.STATUS_DONE;
import static app.organicmaps.sdk.downloader.CountryItem.STATUS_DOWNLOADABLE;
import static app.organicmaps.sdk.downloader.CountryItem.STATUS_ENQUEUED;
import static app.organicmaps.sdk.downloader.CountryItem.STATUS_FAILED;
import static app.organicmaps.sdk.downloader.CountryItem.STATUS_PARTLY;
import static app.organicmaps.sdk.downloader.CountryItem.STATUS_PROGRESS;
import static app.organicmaps.sdk.downloader.CountryItem.STATUS_UPDATABLE;

import android.content.Context;
import android.view.View;
import android.widget.Button;
import app.organicmaps.R;
import app.organicmaps.sdk.downloader.CountryItem;
import app.organicmaps.sdk.downloader.MapManager;
import app.organicmaps.sdk.downloader.UpdateInfo;
import app.organicmaps.sdk.util.StringUtils;
import app.organicmaps.util.UiUtils;
import com.google.android.material.floatingactionbutton.FloatingActionButton;

class BottomPanel
{
  private final DownloaderFragment mFragment;
  private final FloatingActionButton mFab;
  private final Button mButton;

  private final View.OnClickListener mDownloadListener = new View.OnClickListener() {
    @Override
    public void onClick(View v)
    {
      MapManagerHelper.warn3gAndDownload(v.getContext(), mFragment.getCurrentRoot(), null);
    }
  };

  private final View.OnClickListener mUpdateListener = new View.OnClickListener() {
    @Override
    public void onClick(View v)
    {
      final String country = mFragment.getCurrentRoot();
      final Context context = v.getContext();
      MapManagerHelper.warnOn3gUpdate(context, country, () -> MapManagerHelper.startUpdate(context, country));
    }
  };

  private final View.OnClickListener mCancelListener = new View.OnClickListener() {
    @Override
    public void onClick(View v)
    {
      MapManager.nativeCancel(mFragment.getCurrentRoot());
      mFragment.getAdapter().refreshData();
    }
  };

  private final View.OnClickListener mRetryListener = new View.OnClickListener() {
    @Override
    public void onClick(View v)
    {
      MapManagerHelper.warn3gAndRetry(v.getContext(), mFragment.getCurrentRoot(), null);
    }
  };

  BottomPanel(DownloaderFragment fragment, View frame)
  {
    mFragment = fragment;

    mFab = frame.findViewById(R.id.fab);
    mFab.setOnClickListener(v -> {
      if (mFragment.getAdapter() != null)
        mFragment.getAdapter().setAvailableMapsMode();
      update();
    });

    mButton = frame.findViewById(R.id.action);
  }

  private void setUpdateAllState(UpdateInfo info)
  {
    mButton.setText(
        StringUtils.formatUsingUsLocale("%s (%s)", mFragment.getString(R.string.downloader_update_all_button),
                                        StringUtils.getFileSizeString(mFragment.requireContext(), info.totalSize)));
    mButton.setOnClickListener(mUpdateListener);
  }

  private void setDownloadAllState()
  {
    mButton.setText(R.string.downloader_download_all_button);
    mButton.setOnClickListener(mDownloadListener);
  }

  private void setRetryFailedStates()
  {
    mButton.setText(R.string.downloader_retry);
    mButton.setOnClickListener(mRetryListener);
  }

  private void setCancelState()
  {
    mButton.setText(R.string.downloader_cancel_all);
    mButton.setOnClickListener(mCancelListener);
  }

  public void update()
  {
    final DownloaderAdapter adapter = mFragment.getAdapter();
    boolean showFab = false;
    boolean showButton = false;

    if (!adapter.isSearchResultsMode())
    {
      final String root = adapter.getCurrentRootId();
      final boolean myMapsMode = adapter.isMyMapsMode();

      showButton = myMapsMode || !CountryItem.isRoot(root);
      if (showButton)
      {
        final int status = MapManager.nativeGetStatus(root);
        // Hide FAB when all maps are already downloaded - nothing new to download.
        showFab = myMapsMode && status != STATUS_DONE;
        switch (status)
        {
        case STATUS_UPDATABLE -> setUpdateAllState(MapManager.nativeGetUpdateInfo(root));
        case STATUS_PROGRESS, STATUS_APPLYING, STATUS_ENQUEUED -> setCancelState();
        case STATUS_FAILED -> setRetryFailedStates();
        case STATUS_DONE -> showButton = false;
        case STATUS_DOWNLOADABLE, STATUS_PARTLY ->
        {
          // My Maps lists downloaded maps only, so there is nothing to offer here.
          if (myMapsMode)
            showButton = false;
          else
            setDownloadAllState();
        }
        default ->
        {
          if (myMapsMode)
            throw new IllegalArgumentException("Inappropriate status for \"" + root + "\": " + status);
          setDownloadAllState();
        }
        }
      }
    }

    UiUtils.showIf(showFab, mFab);
    UiUtils.showIf(showButton, mButton);
    mFragment.requireView().requestApplyInsets();
  }
}
