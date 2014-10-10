package com.mapswithme.country;

import android.app.Activity;
import android.view.View;

import com.mapswithme.maps.guides.GuideInfo;

class DownloadAdapter extends BaseDownloadAdapter implements CountryTree.CountryTreeListener
{
  protected CountryItem[] mItems;

  public DownloadAdapter(Activity activity)
  {
    super(activity);
    CountryTree.setDefaultRoot();
    fillList();
  }

  /**
   * Get items from native and notify dataset changes.
   */
  protected void fillList()
  {
    final int count = CountryTree.getChildCount();
    if (count > 0)
    {
      mItems = new CountryItem[count];
      for (int i = 0; i < count; ++i)
      {
        mItems[i] = new CountryItem(CountryTree.getChildName(i), CountryTree.getLeafStatus(i),
            CountryTree.getLeafOptions(i), !CountryTree.isLeaf(i));
      }
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

    if (item.hasChildren())      // expand next level
      expandGroup(position);
    else
      showCountryContextMenu(item, view, position);
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
  protected void showCountry(int position)
  {
    CountryTree.showLeafOnMap(position);
    resetCountryListener();
    mActivity.finish();
  }

  protected void expandGroup(int position)
  {
    CountryTree.setChildAsRoot(position);
    fillList();
  }

  @Override
  public boolean onBackPressed()
  {
    if (!CountryTree.hasParent())
      return false;

    CountryTree.setParentAsRoot();
    fillList();
    return true;
  }

  @Override
  protected void deleteCountry(int position, int options)
  {
    CountryTree.deleteCountry(position, options);
  }

  @Override
  protected long[] getItemSizes(int position, int options)
  {
    return CountryTree.getLeafSize(position, options);
  }

  @Override
  protected long[] getRemoteItemSizes(int position)
  {
    return CountryTree.getRemoteLeafSizes(position);
  }

  @Override
  protected long[] getDownloadableItemSizes(int position)
  {
    return CountryTree.getDownloadableLeafSize(position);
  }

  @Override
  protected GuideInfo getGuideInfo(int position)
  {
    return CountryTree.getLeafGuideInfo(position);
  }

  @Override
  protected void cancelDownload(int position)
  {
    CountryTree.cancelDownloading(position);
  }

  @Override
  protected void setCountryListener()
  {
    CountryTree.setListener(this);
  }

  @Override
  protected void resetCountryListener()
  {
    CountryTree.resetListener();
  }

  @Override
  protected void updateCountry(int position, int options)
  {
    CountryTree.downloadCountry(position, options);
  }

  @Override
  protected void downloadCountry(int position, int options)
  {
    CountryTree.downloadCountry(position, options);
  }

  @Override
  public void onItemProgressChanged(int position, long[] sizes)
  {
    onCountryProgress(position, sizes[0], sizes[1]);
  }

  @Override
  public void onItemStatusChanged(int position)
  {
    onCountryStatusChanged(position);
  }
}
