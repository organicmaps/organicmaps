package com.mapswithme.maps.settings;

import android.os.Bundle;
import android.view.View;
import android.widget.ListView;

import com.mapswithme.maps.base.MapsWithMeBaseListActivity;
import com.mapswithme.util.StoragePathManager;

public class StoragePathActivity extends MapsWithMeBaseListActivity
{
  private StoragePathManager m_pathManager = new StoragePathManager();

  private StoragePathManager.StoragePathAdapter getAdapter()
  {
    return (StoragePathManager.StoragePathAdapter)getListView().getAdapter();
  }

  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
  }

  @Override
  protected void onListItemClick(final ListView l, View v, final int position, long id)
  {
    // Do not process clicks on header items.
    if (position != 0)
      getAdapter().onItemClick(position);
  }

  @Override
  protected void onResume()
  {
    super.onResume();
    m_pathManager.StartExtStorageWatching(this, null);
    setListAdapter(m_pathManager.GetAdapter());
  }

  @Override
  protected void onPause()
  {
    super.onPause();
    m_pathManager.StopExtStorageWatching();
  }
}
