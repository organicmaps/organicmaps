package com.mapswithme.maps.ugc.routes;

import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseToolbarActivity;

public class UgcRouteTagsActivity extends BaseToolbarActivity
{
  public static final String EXTRA_TAGS = "selected_tags";
  private static final String FRAGMENT_TAG = UgcRouteTagsActivity.class.getName();

  @Override
  protected void safeOnCreate(@Nullable Bundle savedInstanceState)
  {
    super.safeOnCreate(savedInstanceState);
    Fragment fragment = getSupportFragmentManager().findFragmentByTag(FRAGMENT_TAG);
    if (fragment == null)
      getSupportFragmentManager()
          .beginTransaction()
          .add(R.id.fragment_container, new UgcRouteTagsFragment(), FRAGMENT_TAG)
          .commit();
  }

  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return null;
  }
}
