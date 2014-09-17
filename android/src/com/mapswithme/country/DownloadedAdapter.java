package com.mapswithme.country;

import android.app.Activity;
import android.view.View;

import com.mapswithme.maps.MapStorage;

public class DownloadedAdapter extends BaseDownloadAdapter
{
  public DownloadedAdapter(Activity activity)
  {
    super(activity);
  }

  @Override
  protected void fillList()
  {

  }

  @Override
  protected void expandGroup(int position)
  {
    //
  }

  @Override
  public boolean onBackPressed()
  {
    return false;
  }

  @Override
  public int getCount()
  {
    return 0;
  }

  @Override
  public CountryItem getItem(int position)
  {
    return null;
  }

  @Override
  protected int getItemPosition(MapStorage.Index idx)
  {
    return 0;
  }

  @Override
  protected boolean isRoot()
  {
    return false;
  }

  @Override
  public void onItemClick(int position, View view)
  {
    //
  }
}
