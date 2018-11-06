package com.mapswithme.maps.ugc.routes;

import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmFragmentActivity;

public class UgcRouteSharingOptionsActivity extends BaseMwmFragmentActivity
{
  public static final int REQUEST_CODE = 307;

  @Override
  protected void safeOnCreate(@Nullable Bundle savedInstanceState)
  {
    super.safeOnCreate(savedInstanceState);
    checkForResultCall();
  }

  private void checkForResultCall()
  {
    if (getCallingActivity() == null)
      throw new IllegalStateException("UgcRouteSharingOptionsActivity must be started for result");
  }

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
