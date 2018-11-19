package com.mapswithme.maps.ugc.routes;

import android.support.v4.app.Fragment;

import com.mapswithme.maps.base.BaseMwmFragmentActivity;

public class SendLinkPlaceholderActivity extends BaseMwmFragmentActivity
{
  @Override
  protected boolean useTransparentStatusBar()
  {
    return false;
  }

  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return SendLinkPlaceholderFragment.class;
  }
}
