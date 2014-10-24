package com.mapswithme.country;

import android.app.Activity;
import android.view.View;

import com.mapswithme.maps.guides.GuideInfo;

class DownloadAdapter extends BaseDownloadAdapter implements CountryTree.CountryTreeListener
{
  public DownloadAdapter(Activity activity)
  {
    super(activity);
    CountryTree.setDefaultRoot();
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
    return CountryTree.getChildCount();
  }

  @Override
  public CountryItem getItem(int position)
  {
    return CountryTree.getChildItem(position);
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
    notifyDataSetChanged();
  }

  @Override
  public boolean onBackPressed()
  {
    if (!CountryTree.hasParent())
      return false;

    CountryTree.setParentAsRoot();
    notifyDataSetChanged();
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
    return new long[] { CountryTree.getLeafSize(position, options, true),
                        CountryTree.getLeafSize(position, options, false) };
  }

  @Override
  protected long[] getRemoteItemSizes(int position)
  {
    long mapOnly = CountryTree.getLeafSize(position, StorageOptions.MAP_OPTION_MAP_ONLY, false);
    return new long[] { mapOnly, mapOnly + CountryTree.getLeafSize(position, StorageOptions.MAP_OPTION_CAR_ROUTING, false) };
  }

  @Override
  protected long[] getDownloadableItemSizes(int position)
  {
    return new long[] { CountryTree.getLeafSize(position, -1, true),
                        CountryTree.getLeafSize(position, -1, false) };
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
