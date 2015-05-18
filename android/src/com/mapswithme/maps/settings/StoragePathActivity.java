package com.mapswithme.maps.settings;

import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentTransaction;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmFragmentActivity;

public class StoragePathActivity extends BaseMwmFragmentActivity
{
  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);

    getToolbar().setTitle(R.string.maps_storage);

    FragmentTransaction transaction = getSupportFragmentManager().beginTransaction();
    Fragment fragment = Fragment.instantiate(this, StoragePathFragment.class.getName(), getIntent().getExtras());
    transaction.replace(android.R.id.content, fragment, "fragment");
    transaction.commit();
  }
}
