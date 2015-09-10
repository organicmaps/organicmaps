package com.mapswithme.maps.settings;

import android.app.Activity;
import android.support.annotation.LayoutRes;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.CheckedTextView;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.util.Constants;

import java.util.ArrayList;
import java.util.List;

class StoragePathAdapter extends BaseAdapter
{
  private static final int TYPE_HEADER = 0;
  private static final int TYPE_ITEM = 1;
  private static final int HEADERS_COUNT = 1;
  private static final int TYPES_COUNT = 2;

  private StoragePathManager mStoragePathManager;
  private final LayoutInflater mInflater;
  private final Activity mContext;

  private List<StorageItem> mItems;
  private int mCurrentStorageIndex = -1;
  private long mSizeNeeded;

  public StoragePathAdapter(StoragePathManager storagePathManager, Activity context)
  {
    mStoragePathManager = storagePathManager;
    mContext = context;
    mInflater = mContext.getLayoutInflater();
  }

  @Override
  public int getItemViewType(int position)
  {
    return (position == 0 ? TYPE_HEADER : TYPE_ITEM);
  }

  @Override
  public int getViewTypeCount()
  {
    return TYPES_COUNT;
  }

  @Override
  public int getCount()
  {
    return (mItems != null ? mItems.size() + HEADERS_COUNT : HEADERS_COUNT);
  }

  @Override
  public StorageItem getItem(int position)
  {
    return (getItemViewType(position) == TYPE_HEADER ? null : mItems.get(getStorageIndex(position)));
  }

  @Override
  public long getItemId(int position)
  {
    return position;
  }

  @Override
  public boolean isEnabled(int position)
  {
    return getItemViewType(position) == TYPE_ITEM;
  }

  @Override
  public View getView(int position, View convertView, ViewGroup parent)
  {
    int viewType = getItemViewType(position);
    if (convertView == null)
      convertView = mInflater.inflate(getLayoutForType(viewType), parent, false);

    switch (getItemViewType(position))
    {
    case TYPE_HEADER:
      TextView view = (TextView) convertView;
      view.setText(mContext.getString(R.string.maps) + ": " + getSizeString(mSizeNeeded));
      break;
    case TYPE_ITEM:
      int storageIndex = getStorageIndex(position);
      StorageItem item = mItems.get(storageIndex);

      if (convertView == null)
        convertView = mInflater.inflate(R.layout.item_storage, parent, false);

      CheckedTextView checkedView = (CheckedTextView) convertView;
      checkedView.setText(item.mPath + ": " + getSizeString(item.mFreeSize));
      checkedView.setChecked(storageIndex == mCurrentStorageIndex);
      checkedView.setEnabled(storageIndex == mCurrentStorageIndex || isStorageBigEnough(storageIndex));
      break;
    }

    return convertView;
  }

  private
  @LayoutRes
  int getLayoutForType(int viewType)
  {
    return viewType == 0 ? R.layout.item_storage_title : R.layout.item_storage;
  }

  public void onItemClick(int position)
  {
    final int index = getStorageIndex(position);
    if (isStorageBigEnough(index) && index != mCurrentStorageIndex)
      mStoragePathManager.changeStorage(index);
  }

  public void updateList(ArrayList<StorageItem> items, int currentItemIndex, long dirSize)
  {
    mSizeNeeded = dirSize;
    mItems = items;
    mCurrentStorageIndex = currentItemIndex;

    notifyDataSetChanged();
  }

  private String getSizeString(long size)
  {
    final String arrS[] = {"Kb", "Mb", "Gb"};

    long current = Constants.KB;
    int i = 0;
    for (; i < arrS.length; ++i)
    {
      final long bound = Constants.KB * current;
      if (size < bound)
        break;
      else
        current = bound;
    }

    // left 1 digit after the comma and add postfix string
    return String.format("%.1f %s", (double) size / (double) current, arrS[i]);
  }

  private boolean isStorageBigEnough(int index)
  {
    return mItems.get(index).mFreeSize >= mSizeNeeded;
  }

  private int getStorageIndex(int position)
  {
    return position - HEADERS_COUNT;
  }

}
