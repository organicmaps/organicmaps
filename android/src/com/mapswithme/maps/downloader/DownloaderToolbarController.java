package com.mapswithme.maps.downloader;

import android.app.Activity;
import android.content.Intent;
import android.text.TextUtils;
import android.view.View;

import com.mapswithme.maps.R;
import com.mapswithme.maps.widget.SearchToolbarController;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.statistics.Statistics;

class DownloaderToolbarController extends SearchToolbarController
{
  private final DownloaderFragment mFragment;

  DownloaderToolbarController(View root, Activity activity, DownloaderFragment fragment)
  {
    super(root, activity);
    mFragment = fragment;
    setHint(R.string.downloader_search_field_hint);
  }

  @Override
  public void onUpClick()
  {
    requireActivity().onBackPressed();
  }

  @Override
  protected void onTextChanged(String query)
  {
    if (!mFragment.isAdded() || !mFragment.shouldShowSearch())
      return;

    if (TextUtils.isEmpty(query))
      mFragment.cancelSearch();
    else
      mFragment.startSearch();
  }

  public void update()
  {
    boolean showSearch = mFragment.shouldShowSearch();
    String title = (showSearch ? "" : mFragment.getAdapter().getCurrentRootName());

    showSearchControls(showSearch);
    if (!showSearch)
      UiUtils.setupHomeUpButtonAsNavigationIcon(getToolbar(), mNavigationClickListener);
    else
      UiUtils.clearHomeUpButton(getToolbar());
    setTitle(title);
  }

  @Override
  protected int getVoiceInputPrompt()
  {
    return R.string.downloader_search_field_hint;
  }

  @Override
  protected void startVoiceRecognition(Intent intent, int code)
  {
    mFragment.startActivityForResult(intent, code);
  }

  @Override
  protected void onQueryClick(String query)
  {
    super.onQueryClick(query);
    boolean isDownloadNewMapsMode = mFragment.getAdapter().canGoUpwards();
    Statistics.ParameterBuilder params = Statistics
        .params().add(Statistics.EventParam.SCREEN, isDownloadNewMapsMode
                                                    ? Statistics.ParamValue.DOWNLOAD
                                                    : Statistics.ParamValue.PLUS);
    Statistics.INSTANCE.trackEvent(Statistics.EventName.DOWNLOADER_SEARCH_CLICK, params);
  }

  @Override
  protected boolean supportsVoiceSearch()
  {
    return true;
  }
}
