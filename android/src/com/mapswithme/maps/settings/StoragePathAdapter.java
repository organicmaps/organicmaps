package com.mapswithme.maps.settings;

import android.app.Activity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.CheckedTextView;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.util.Constants;
import com.mapswithme.util.Utils;

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
  private final int mListItemHeight;

  private List<StorageItem> mItems;
  private int mCurrent = -1;
  private long mSizeNeeded;

  public StoragePathAdapter(StoragePathManager storagePathManager, Activity context)
  {
    mStoragePathManager = storagePathManager;
    mContext = context;
    mInflater = mContext.getLayoutInflater();

    mListItemHeight = (int) Utils.getAttributeDimension(context, android.R.attr.listPreferredItemHeight);
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
    return (position == 0 ? null : mItems.get(getIndexFromPos(position)));
  }

  @Override
  public long getItemId(int position)
  {
    return position;
  }

  @Override
  public View getView(int position, View convertView, ViewGroup parent)
  {
    // 1. It's a strange thing, but when I tried to use setClickable,
    // all the views become nonclickable.
    // 2. I call setMinimumHeight(listPreferredItemHeight)
    // because standard item's height is unknown.

    switch (getItemViewType(position))
    {
    case TYPE_HEADER:
      if (convertView == null)
      {
        convertView = mInflater.inflate(android.R.layout.simple_list_item_1, parent, false);
        convertView.setMinimumHeight(mListItemHeight);
      }

      final TextView v = (TextView) convertView;
      v.setText(mContext.getString(R.string.maps) + ": " + getSizeString(mSizeNeeded));
      break;
    case TYPE_ITEM:
      final int index = getIndexFromPos(position);
      final StorageItem item = mItems.get(index);

      if (convertView == null)
      {
        convertView = mInflater.inflate(android.R.layout.simple_list_item_single_choice, parent, false);
        convertView.setMinimumHeight(mListItemHeight);
      }

      final CheckedTextView ctv = (CheckedTextView) convertView;
      ctv.setText(item.mPath + ": " + getSizeString(item.mSize));
      ctv.setChecked(index == mCurrent);
      ctv.setEnabled((index == mCurrent) || isAvailable(index));
      break;
    }

    return convertView;
  }

  public void onItemClick(int position)
  {
    final int index = getIndexFromPos(position);
    if (isAvailable(index))
      mStoragePathManager.onStorageItemClick(index);
  }

  public void updateList(ArrayList<StorageItem> items, int currentItemIndex, long dirSize)
  {
    mSizeNeeded = dirSize;
    mItems = items;
    mCurrent = currentItemIndex;

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

  private boolean isAvailable(int index)
  {
    assert (index >= 0 && index < mItems.size());
    return ((mCurrent != index) && (mItems.get(index).mSize >= mSizeNeeded));
  }

  private int getIndexFromPos(int position)
  {
    final int index = position - HEADERS_COUNT;
    assert (index >= 0 && index < mItems.size());
    return index;
  }

  public static class StorageItem
  {
    public final String mPath;
    public final long mSize;

    StorageItem(String path, long size)
    {
      mPath = path;
      mSize = size;
    }

    @Override
    public boolean equals(Object o)
    {
      if (o == this)
        return true;
      if (o == null || !(o instanceof StorageItem))
        return false;
      StorageItem other = (StorageItem) o;
      return mSize == other.mSize && mPath.equals(other.mPath);
    }

    @Override
    public int hashCode()
    {
      return Long.valueOf(mSize).hashCode();
    }
  }
}
