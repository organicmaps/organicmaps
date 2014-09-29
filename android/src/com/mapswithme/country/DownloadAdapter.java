package com.mapswithme.country;

import android.app.Activity;
import android.util.Log;
import android.view.View;

import com.mapswithme.maps.MapStorage;

class DownloadAdapter extends BaseDownloadAdapter
{
  protected MapStorage.Index mIdx;
  protected CountryItem[] mItems;

  public DownloadAdapter(Activity activity)
  {
    super(activity);
    mIdx = new MapStorage.Index();
    fillList();
  }

  /**
   * Updates list items from current index.
   */
  protected void fillList()
  {
    final int count = MapStorage.INSTANCE.countriesCount(mIdx);
    if (count > 0)
    {
      mItems = new CountryItem[count];
      for (int i = 0; i < count; ++i)
        mItems[i] = new CountryItem(mIdx.getChild(i));
    }

    notifyDataSetChanged();
  }

  @Override
  public void onItemClick(int position, View view)
  {
    if (position >= getCount())
      return; // we have reports at GP that it crashes.

    final CountryItem item = getItem(position);
    if (item == null)
      return;
    if (item.getStatus() < 0)
    {
      // expand next level
      expandGroup(position);
    }
    else
      onCountryMenuClicked(item, view);
  }

  @Override
  public int getCount()
  {
    return (mItems != null ? mItems.length : 0);
  }

  @Override
  public CountryItem getItem(int position)
  {
    return mItems[position];
  }

  @Override
  protected int getItemPosition(MapStorage.Index idx)
  {
    if (mIdx.isChild(idx))
    {
      final int position = idx.getPosition();
      if (position >= 0 && position < getCount())
        return position;
      else
        Log.e(DownloadActivity.TAG, "Incorrect item position for: " + idx.toString());
    }
    return -1;
  }

  protected void expandGroup(int position)
  {
    mIdx = mIdx.getChild(position);
    fillList();
  }

  @Override
  public boolean onBackPressed()
  {
    if (mIdx.isRoot())
      return false;

    mIdx = mIdx.getParent();

    fillList();
    return true;
  }

  protected boolean isRoot()
  {
    return mIdx.isRoot();
  }
}
