package com.mapswithme.maps.purchase;

import android.support.v4.app.Fragment;

import com.mapswithme.maps.base.BaseMwmFragmentActivity;

public class AllPassPremiumActivity extends BaseMwmFragmentActivity
{
  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return AllPassPremiumPagerFragment.class;
  }
}
