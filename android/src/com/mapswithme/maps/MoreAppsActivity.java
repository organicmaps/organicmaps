package com.mapswithme.maps;

import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentTransaction;

import com.mapswithme.maps.base.MWMFragmentActivity;

public class MoreAppsActivity extends MWMFragmentActivity
{
  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);

    setTitle(getString(R.string.more_apps_title));

    FragmentTransaction transaction = getSupportFragmentManager().beginTransaction();
    Fragment fragment = Fragment.instantiate(this, MoreAppsFragment.class.getName(), getIntent().getExtras());
    transaction.replace(android.R.id.content, fragment, "fragment");
    transaction.commit();
  }
}
