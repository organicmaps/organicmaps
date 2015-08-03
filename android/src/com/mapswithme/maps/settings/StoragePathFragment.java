package com.mapswithme.maps.settings;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.view.View;
import android.widget.ListView;

import com.mapswithme.maps.base.BaseMwmListFragment;
import com.mapswithme.util.UiUtils;

public class StoragePathFragment extends BaseMwmListFragment implements StoragePathManager.MoveFilesListener
{
  private StoragePathManager mPathManager = new StoragePathManager();
  private StoragePathAdapter mAdapter;

  private StoragePathAdapter getAdapter()
  {
    return (StoragePathAdapter) getListView().getAdapter();
  }

  @Override
  public void onListItemClick(final ListView l, View v, final int position, long id)
  {
    getAdapter().onItemClick(position);
  }

  @Override
  public void onResume()
  {
    super.onResume();
    BroadcastReceiver receiver = new BroadcastReceiver()
    {
      @Override
      public void onReceive(Context context, Intent intent)
      {
        if (mAdapter != null)
          mAdapter.updateList(mPathManager.getStorageItems(), mPathManager.getCurrentStorageIndex(), StoragePathManager.getMwmDirSize());
      }
    };
    mPathManager.startExternalStorageWatching(getActivity(), receiver, this);
    initAdapter();
    mAdapter.updateList(mPathManager.getStorageItems(), mPathManager.getCurrentStorageIndex(), StoragePathManager.getMwmDirSize());
    setListAdapter(mAdapter);
  }

  @Override
  public void onPause()
  {
    super.onPause();
    mPathManager.stopExternalStorageWatching();
  }

  private void initAdapter()
  {
    if (mAdapter == null)
      mAdapter = new StoragePathAdapter(mPathManager, getActivity());
  }

  @Override
  public void moveFilesFinished(String newPath)
  {
    mAdapter.updateList(mPathManager.getStorageItems(), mPathManager.getCurrentStorageIndex(), StoragePathManager.getMwmDirSize());
  }

  @Override
  public void moveFilesFailed(int errorCode)
  {
    UiUtils.showAlertDialog(getActivity(), "Failed to move maps with internal error :" + errorCode + ". Please contact us at bugs@maps.me and send this error code.");
  }
}
