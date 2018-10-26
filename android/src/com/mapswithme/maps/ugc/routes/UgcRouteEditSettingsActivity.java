package com.mapswithme.maps.ugc.routes;

import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentManager;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmFragmentActivity;

public class UgcRouteEditSettingsActivity extends BaseMwmFragmentActivity
{
  public static final String EXTRA_BOOKMARK_CATEGORY = "bookmark_category";

  private static final String FRAGMENT_TAG = "edit_settings_fragment_tag";

  @Override
  protected void safeOnCreate(@Nullable Bundle savedInstanceState)
  {
    super.safeOnCreate(savedInstanceState);
    setContentView(R.layout.ugc_route_edit_settings_activity);
    addSettingsFragmentIfAbsent();
  }

  private void addSettingsFragmentIfAbsent()
  {
    FragmentManager fm = getSupportFragmentManager();
    UgcRouteEditSettingsFragment fragment = (UgcRouteEditSettingsFragment) fm.findFragmentByTag(FRAGMENT_TAG);

    if (fragment == null)
    {
      fragment = UgcRouteEditSettingsFragment.makeInstance(getIntent().getExtras());
      fm.beginTransaction().add(R.id.fragment_container, fragment, FRAGMENT_TAG).commit();
    }
  }

  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return null;
  }
}
