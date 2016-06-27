package com.mapswithme.maps.settings;

import android.app.Activity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.CheckedTextView;

import java.util.ArrayList;
import java.util.List;

import com.mapswithme.maps.R;

class StoragePathAdapter extends BaseAdapter
{
  private final StoragePathManager mStoragePathManager;
  private final LayoutInflater mInflater;

  private final List<StorageItem> mItems = new ArrayList<>();
  private int mCurrentStorageIndex = -1;
  private long mSizeNeeded;

  StoragePathAdapter(StoragePathManager storagePathManager, Activity context)
  {
    mStoragePathManager = storagePathManager;
    mInflater = context.getLayoutInflater();
  }

  @Override
  public int getCount()
  {
    return (mItems == null ? 0 : mItems.size());
  }

  @Override
  public StorageItem getItem(int position)
  {
    return mItems.get(position);
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
      convertView = mInflater.inflate(R.layout.item_storage, parent, false);

    StorageItem item = mItems.get(position);
    CheckedTextView checkedView = (CheckedTextView) convertView;
    checkedView.setText(item.mPath + ": " + StoragePathFragment.getSizeString(item.mFreeSize));
    checkedView.setChecked(position == mCurrentStorageIndex);
    checkedView.setEnabled(position == mCurrentStorageIndex || isStorageBigEnough(position));

    return convertView;
  }

  public void onItemClick(int position)
  {
    if (isStorageBigEnough(position) && position != mCurrentStorageIndex)
      mStoragePathManager.changeStorage(position);
  }

  public void update(List<StorageItem> items, int currentItemIndex, long dirSize)
  {
    mSizeNeeded = dirSize;
    mItems.clear();
    mItems.addAll(items);
    mCurrentStorageIndex = currentItemIndex;

    notifyDataSetChanged();
  }

  private boolean isStorageBigEnough(int index)
  {
    return mItems.get(index).mFreeSize >= mSizeNeeded;
  }
}
