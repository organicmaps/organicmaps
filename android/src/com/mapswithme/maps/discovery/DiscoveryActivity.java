package com.mapswithme.maps.discovery;

import android.support.v4.app.Fragment;

import com.mapswithme.maps.base.BaseMwmFragmentActivity;

public class DiscoveryActivity extends BaseMwmFragmentActivity
{
  public static final String EXTRA_DISCOVERY_OBJECT = "extra_discovery_object";
  public static final String ACTION_ROUTE_TO = "action_route_to";
  public static final String ACTION_SHOW_ON_MAP = "action_show_on_map";

  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return DiscoveryFragment.class;
  }
}
