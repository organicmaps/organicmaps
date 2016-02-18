package com.mapswithme.maps.downloader;

import android.app.Activity;
import android.content.Intent;
import android.text.TextUtils;
import android.view.View;

import com.mapswithme.maps.R;
import com.mapswithme.maps.widget.SearchToolbarController;
import com.mapswithme.util.UiUtils;

class DownloaderToolbarController extends SearchToolbarController
{
  private final DownloaderFragment mFragment;

  private final View mDownloadAll;
  private final View mUpdateAll;
  private final View mCancelAll;

  DownloaderToolbarController(View root, Activity activity, DownloaderFragment fragment)
  {
    super(root, activity);
    mFragment = fragment;

    mDownloadAll = mToolbar.findViewById(R.id.download_all);
    mUpdateAll = mToolbar.findViewById(R.id.update_all);
    mCancelAll = mToolbar.findViewById(R.id.cancel_all);

    mDownloadAll.setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        MapManager.nativeDownload(mFragment.getAdapter().getCurrentParent());
      }
    });

    mUpdateAll.setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        MapManager.nativeUpdate(mFragment.getAdapter().getCurrentParent());
      }
    });

    mCancelAll.setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        MapManager.nativeCancel(mFragment.getAdapter().getCurrentParent());
      }
    });
  }

  @Override
  protected void onTextChanged(String query)
  {
    if (!mFragment.isAdded())
      return;

    if (TextUtils.isEmpty(query))
    {
      mFragment.cancelSearch();
      return;
    }

    mFragment.startSearch();
  }

  @Override
  public void onUpClick()
  {
    mFragment.onBackPressed();
  }

  public void update()
  {
    boolean search = hasQuery();
    boolean cancel = MapManager.nativeIsDownloading();

    boolean update = (!search && !cancel);
    if (update)
    {
      // TODO (trashkalmar): Use appropriate function
      update = false;
    }

    boolean onTop = !mFragment.getAdapter().canGoUpdwards();

    boolean download = (!search && !cancel && !update && !onTop);
    if (download)
    {
      // TODO (trashkalmar): Use appropriate function
      download = true;
    }

    UiUtils.showIf(cancel, mCancelAll);
    UiUtils.showIf(update, mUpdateAll);
    UiUtils.showIf(download, mDownloadAll);
    showControls(onTop);
  }

  @Override
  protected int getVoiceInputPrompt()
  {
    return R.string.search_map;
  }

  @Override
  protected void startVoiceRecognition(Intent intent, int code)
  {
    mFragment.startActivityForResult(intent, code);
  }
}
