package com.mapswithme.maps.settings;

import android.support.v4.app.Fragment;

import com.mapswithme.maps.base.BaseMwmFragmentActivity;

public class StoragePathActivity extends BaseMwmFragmentActivity
{
  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return StoragePathFragment.class;
  }
}
