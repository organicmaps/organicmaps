package com.mapswithme.maps.ugc.routes;

import android.support.v4.app.Fragment;

import com.mapswithme.maps.base.BaseToolbarActivity;

public class UgcRoutePropertiesActivity extends BaseToolbarActivity
{
  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return UgcRoutePropertiesFragment.class;
  }
}
