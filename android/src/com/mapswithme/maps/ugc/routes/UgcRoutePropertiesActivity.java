package com.mapswithme.maps.ugc.routes;

import android.support.v4.app.Fragment;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmFragmentActivity;

public class UgcRoutePropertiesActivity extends BaseMwmFragmentActivity
{

  @Override
  protected int getContentLayoutResId()
  {
    return R.layout.fragment_container_layout;
  }

  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return UgcRoutePropertiesFragment.class;
  }
}
