package com.mapswithme.maps.ugc.routes;

import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentManager;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmFragmentActivity;

public class UgcRouteSharingOptionsActivity extends BaseMwmFragmentActivity
{

  private static final String SHARING_OPTIONS_FRAGMENT_TAG = "sharing_options_fragment";

  @Override
  protected void safeOnCreate(@Nullable Bundle savedInstanceState)
  {
    super.safeOnCreate(savedInstanceState);
    setContentView(R.layout.ugc_route_sharing_options_activity);
    FragmentManager fm = getSupportFragmentManager();
    Fragment fragment = fm.findFragmentByTag(SHARING_OPTIONS_FRAGMENT_TAG);
    if (fragment == null)
      fm.beginTransaction()
        .add(R.id.fragment_container, new UgcSharingOptionsFragment(), SHARING_OPTIONS_FRAGMENT_TAG)
        .commit();
  }

  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return null;
  }
}
