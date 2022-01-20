package com.mapswithme.maps.editor;

import androidx.fragment.app.Fragment;

import com.mapswithme.maps.base.BaseMwmFragmentActivity;

public class ProfileActivity extends BaseMwmFragmentActivity
{
  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return ProfileFragment.class;
  }
}
