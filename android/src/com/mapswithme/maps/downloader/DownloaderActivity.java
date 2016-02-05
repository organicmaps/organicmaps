package com.mapswithme.maps.downloader;

import android.os.Bundle;
import android.support.v4.app.Fragment;

import com.mapswithme.maps.base.BaseMwmFragmentActivity;
import com.mapswithme.maps.base.OnBackPressListener;

public class DownloaderActivity extends BaseMwmFragmentActivity
{
  public static final String EXTRA_OPEN_DOWNLOADED = "open downloaded";

  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return DownloaderFragment.class;
  }

  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);

  }

  @Override
  public void onBackPressed()
  {
    OnBackPressListener fragment = (OnBackPressListener)getSupportFragmentManager().findFragmentById(getFragmentContentResId());
    if (!fragment.onBackPressed())
      super.onBackPressed();
  }
}
