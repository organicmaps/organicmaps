package com.mapswithme.maps.ugc.routes;

import android.support.v4.app.Fragment;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmFragmentActivity;

public class UgcRouteSharingOptionsActivity extends BaseMwmFragmentActivity
{
  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return UgcSharingOptionsFragment.class;
  }

  @Override
  protected int getContentLayoutResId()
  {
    return R.layout.fragment_container_layout;
  }
}
