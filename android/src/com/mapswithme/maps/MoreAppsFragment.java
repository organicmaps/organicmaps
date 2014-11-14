package com.mapswithme.maps;

import android.os.Bundle;

import com.mapswithme.maps.base.MWMListFragment;

public class MoreAppsFragment extends MWMListFragment
{
  @Override
  public void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);

    setListAdapter(new MoreAppsAdapter(getActivity()));
  }
}
