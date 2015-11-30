package com.mapswithme.country;

import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentTransaction;
import android.util.Log;

import com.mapswithme.maps.base.BaseMwmFragmentActivity;

public class DownloadActivity extends BaseMwmFragmentActivity
{
  public static final String EXTRA_OPEN_DOWNLOADED_LIST = "open_downloaded";

  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);

    final String fragmentClassName = DownloadFragment.class.getName();
    FragmentTransaction transaction = getSupportFragmentManager().beginTransaction();
    DownloadFragment downloadFragment = (DownloadFragment) Fragment.instantiate(this, fragmentClassName, getIntent().getExtras());
    transaction.replace(android.R.id.content, downloadFragment, fragmentClassName);
    transaction.commit();
  }

  @Override
  public void onBackPressed()
  {
    DownloadFragment fragment = (DownloadFragment) getSupportFragmentManager().findFragmentByTag(DownloadFragment.class.getName());
    if (fragment != null && fragment.onBackPressed())
      return;

    super.onBackPressed();
  }
}
