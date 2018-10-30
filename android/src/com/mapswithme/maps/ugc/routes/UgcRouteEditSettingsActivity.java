package com.mapswithme.maps.ugc.routes;

import android.support.v4.app.Fragment;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmFragmentActivity;

public class UgcRouteEditSettingsActivity extends BaseMwmFragmentActivity
{
  @Override
  protected int getContentLayoutResId()
  {
    return R.layout.ugc_route_edit_settings_activity;
  }

  @Override
  protected int getFragmentContentResId()
  {
    return R.id.fragment_container;
  }

  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return UgcRouteEditSettingsFragment.class;
  }
}
