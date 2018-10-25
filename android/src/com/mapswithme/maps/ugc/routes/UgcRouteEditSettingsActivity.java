package com.mapswithme.maps.ugc.routes;

import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentManager;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmFragmentActivity;
import com.mapswithme.maps.base.DataObservable;
import com.mapswithme.maps.base.ObservableHost;

public class UgcRouteEditSettingsActivity extends BaseMwmFragmentActivity implements ObservableHost<DataObservable>
{
  public static final String EXTRA_BOOKMARK_CATEGORY = "bookmark_category";

  private static final String FRAGMENT_TAG = "edit_settings_fragment_tag";

  @SuppressWarnings("NullableProblems")
  @NonNull
  private DataObservable mObservable;

  @Override
  protected void safeOnCreate(@Nullable Bundle savedInstanceState)
  {
    super.safeOnCreate(savedInstanceState);
    setContentView(R.layout.ugc_route_edit_settings_activity);
    mObservable = new DataObservable();
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

  @NonNull
  @Override
  public DataObservable getObservable()
  {
    return mObservable;
  }
}
