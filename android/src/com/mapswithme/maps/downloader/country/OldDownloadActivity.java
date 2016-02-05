package com.mapswithme.maps.downloader.country;

import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentTransaction;

import com.mapswithme.maps.base.BaseMwmFragmentActivity;

@Deprecated
public class OldDownloadActivity extends BaseMwmFragmentActivity
{
  public static final String EXTRA_OPEN_DOWNLOADED_LIST = "open_downloaded";

  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);

    final String fragmentClassName = OldDownloadFragment.class.getName();
    FragmentTransaction transaction = getSupportFragmentManager().beginTransaction();
    OldDownloadFragment downloadFragment = (OldDownloadFragment) Fragment.instantiate(this, fragmentClassName, getIntent().getExtras());
    transaction.replace(android.R.id.content, downloadFragment, fragmentClassName);
    transaction.commit();
  }

  @Override
  public void onBackPressed()
  {
    OldDownloadFragment fragment = (OldDownloadFragment) getSupportFragmentManager().findFragmentByTag(OldDownloadFragment.class.getName());
    if (fragment != null && fragment.onBackPressed())
      return;

    super.onBackPressed();
  }
}
