package com.mapswithme.maps.ugc.routes;

import android.support.v4.app.Fragment;

import com.mapswithme.maps.base.BaseMwmFragmentActivity;

public class EditCategoryDescriptionActivity extends BaseMwmFragmentActivity
{
  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return EditCategoryDescriptionFragment.class;
  }
}
