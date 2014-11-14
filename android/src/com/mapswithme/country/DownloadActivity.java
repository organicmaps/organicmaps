package com.mapswithme.country;

import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentTransaction;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.MWMFragmentActivity;


public class DownloadActivity extends MWMFragmentActivity
{
  private DownloadFragment mDownloadFragment;

  public static final String EXTRA_OPEN_DOWNLOADED_LIST = "open_downloaded";

  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);

    setTitle(getString(R.string.bookmarks));

    FragmentTransaction transaction = getSupportFragmentManager().beginTransaction();
    mDownloadFragment = (DownloadFragment) Fragment.instantiate(this, DownloadFragment.class.getName(), getIntent().getExtras());
    transaction.replace(android.R.id.content, mDownloadFragment, "fragment");
    transaction.commit();
  }

  @Override
  public void onBuyPro()
  {
    if (mDownloadFragment != null && mDownloadFragment.isAdded() && !mDownloadFragment.isRemoving() && mDownloadFragment.onBackPressed())
      return;

    super.onBuyPro();
  }
}
