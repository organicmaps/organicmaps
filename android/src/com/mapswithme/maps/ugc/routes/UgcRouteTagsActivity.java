package com.mapswithme.maps.ugc.routes;

import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;

import com.mapswithme.maps.base.BaseToolbarActivity;

public class UgcRouteTagsActivity extends BaseToolbarActivity
{
  public static final String EXTRA_TAGS = "selected_tags";

  @Override
  protected void safeOnCreate(@Nullable Bundle savedInstanceState)
  {
    super.safeOnCreate(savedInstanceState);
  }

  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return UgcRouteTagsFragment.class;
  }
}
