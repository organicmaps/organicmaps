package com.mapswithme.maps;

import android.os.Bundle;

import com.mapswithme.maps.base.MapsWithMeBaseListActivity;

public class MoreAppsActivity extends MapsWithMeBaseListActivity
{
  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);

    setListAdapter(new MoreAppsAdapter(this));
  }
}
