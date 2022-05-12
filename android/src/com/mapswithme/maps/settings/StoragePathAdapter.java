package com.mapswithme.maps.settings;

import android.app.Activity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.CheckedTextView;

import androidx.annotation.NonNull;
import com.mapswithme.maps.R;

import java.util.ArrayList;
import java.util.List;

class StoragePathAdapter extends BaseAdapter
{
  private final StoragePathManager mPathManager;
  private final Activity mActivity;
  private long mSizeNeeded;

  public StoragePathAdapter(StoragePathManager pathManager, Activity activity)
  {
    mPathManager = pathManager;
    mActivity = activity;
  }

  @Override
  public int getCount()
  {
    return (mPathManager.getStorageItems() == null ? 0 : mPathManager.getStorageItems().size());
  }

  @Override
  public StorageItem getItem(int position)
  {
    return mPathManager.getStorageItems().get(position);
  }

  @Override
  public long getItemId(int position)
  {
    return position;
  }

  @Override
  public View getView(int position, View convertView, ViewGroup parent)
  {
    if (convertView == null)
      convertView = mActivity.getLayoutInflater().inflate(R.layout.item_storage, parent, false);

    StorageItem item = mPathManager.getStorageItems().get(position);
    CheckedTextView checkedView = (CheckedTextView) convertView;
    checkedView.setText(item.getFullPath() + ": " + StoragePathFragment.getSizeString(item.getFreeSize()));
    checkedView.setChecked(position == mPathManager.getCurrentStorageIndex());
    checkedView.setEnabled(position == mPathManager.getCurrentStorageIndex() || isStorageBigEnough(position));

    return convertView;
  }

  public void update(long dirSize)
  {
    mSizeNeeded = dirSize;
    notifyDataSetChanged();
  }

  public boolean isStorageBigEnough(int index)
  {
    return mPathManager.getStorageItems().get(index).getFreeSize() >= mSizeNeeded;
  }
}
