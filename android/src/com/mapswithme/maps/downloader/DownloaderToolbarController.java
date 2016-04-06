package com.mapswithme.maps.downloader;

import android.app.Activity;
import android.content.Intent;
import android.text.TextUtils;
import android.view.View;

import com.mapswithme.maps.R;
import com.mapswithme.maps.widget.SearchToolbarController;

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
  protected void onTextChanged(String query)
  {
    if (!mFragment.isAdded() || mFragment.getAdapter().canGoUpdwards())
      return;

    if (TextUtils.isEmpty(query))
      mFragment.cancelSearch();
    else
      mFragment.startSearch();
  }

  @Override
  public void onUpClick()
  {
    if (!mFragment.onBackPressed())
      super.onUpClick();
  }

  public void update(String title)
  {
    showControls(!mFragment.getAdapter().canGoUpdwards());
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
  protected boolean supportsVoiceSearch()
  {
    return true;
  }
}
