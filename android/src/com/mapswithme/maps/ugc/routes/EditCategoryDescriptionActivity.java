package com.mapswithme.maps.ugc.routes;

import androidx.fragment.app.Fragment;

import com.mapswithme.maps.base.BaseMwmFragmentActivity;

public class EditCategoryDescriptionActivity extends BaseMwmFragmentActivity
{
  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return EditCategoryDescriptionFragment.class;
  }
}
