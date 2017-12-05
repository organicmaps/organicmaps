package com.mapswithme.maps.discovery;

import android.support.v4.app.Fragment;

import com.mapswithme.maps.base.BaseMwmFragmentActivity;

public class DiscoveryActivity extends BaseMwmFragmentActivity
{
  public static final String EXTRA_DISCOVERY_OBJECT = "extra_discovery_object";

  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return DiscoveryFragment.class;
  }
}
