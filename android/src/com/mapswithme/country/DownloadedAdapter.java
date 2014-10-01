package com.mapswithme.country;

import android.app.Activity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ListView;

import com.mapswithme.maps.MapStorage;
import com.mapswithme.maps.R;

public class DownloadedAdapter extends BaseDownloadAdapter
{
  private static final int TYPE_HEADER = 5;
  private static final int VIEW_TYPE_COUNT = 6;
  private int mDownloadedCount;
  private int mOutdatedCount;

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
  public View getView(int position, View convertView, ViewGroup parent)
  {
    final int viewType = getItemViewType(position);
    if (viewType == TYPE_HEADER)
    {
      View view;
      ViewHolder holder;
      if (convertView == null)
      {
        view = mInflater.inflate(R.layout.download_item_updated, parent, false);
        holder = new ViewHolder();
        holder.initFromView(view);
        view.setTag(holder);
      }
      else
      {
        view = convertView;
        holder = (ViewHolder) view.getTag();
      }
      if (containsOutdated() && position == 0)
        holder.mName.setText(mActivity.getString(R.string.downloader_outdated_maps));
      else
        holder.mName.setText(mActivity.getString(R.string.downloader_uptodate_maps));

      return view;
    }
    else
    {
      final View view = super.getView(position, convertView, parent);
      final ViewHolder holder = (ViewHolder) view.getTag();
      if (getOutdatedPosition(position) == INVALID_POSITION)
        holder.mPercent.setVisibility(View.GONE);
      else
        holder.mPercent.setVisibility(View.VISIBLE);
      return view;
    }
  }

  @Override
  public int getCount()
  {
    mDownloadedCount = MapStorage.INSTANCE.getDownloadedCountriesCount();
    mOutdatedCount = MapStorage.INSTANCE.getOutdatedCountriesCount();
    return (mDownloadedCount == 0 ? 0 : mDownloadedCount + 1) + (mOutdatedCount == 0 ? 0 : mOutdatedCount + 1);
  }

  @Override
  public CountryItem getItem(int position)
  {
    if (isHeader(position))
      return null;

    int correctedPosition = getOutdatedPosition(position);
    if (correctedPosition != INVALID_POSITION)
      return new CountryItem(MapStorage.INSTANCE.getOutdatedCountry(correctedPosition));

    correctedPosition = getDownloadedPosition(position);
    return new CountryItem(MapStorage.INSTANCE.getDownloadedCountry(correctedPosition));
  }

  @Override
  public int getItemViewType(int position)
  {
    if (isHeader(position))
      return TYPE_HEADER;

    return super.getItemViewType(position);
  }

  private boolean isHeader(int position)
  {
    return (containsOutdated() && position == 0) ||
        (!containsOutdated() && containsDownloaded() && position == 0) ||
        (containsOutdated() && containsDownloaded() && position == mOutdatedCount + 1);
  }

  @Override
  public int getViewTypeCount()
  {
    return VIEW_TYPE_COUNT;
  }

  @Override
  public boolean isEnabled(int position)
  {
    return !isHeader(position) && super.isEnabled(position);
  }

  @Override
  protected int getItemPosition(MapStorage.Index idx)
  {
    return INVALID_POSITION;
  }

  @Override
  protected boolean isRoot()
  {
    return false;
  }

  @Override
  public void onItemClick(int position, View view)
  {
    onCountryMenuClicked(getItem(position), view);
  }

  protected int getOutdatedPosition(int position)
  {
    if (!containsOutdated())
      return INVALID_POSITION;

    if (position <= mOutdatedCount)
      return position - 1;

    return INVALID_POSITION;
  }

  protected int getDownloadedPosition(int position)
  {
    if (!containsDownloaded())
      return INVALID_POSITION;

    final int startNum = 1 + (containsOutdated() ? mOutdatedCount + 1 : 0);
    if (position >= startNum && position < startNum + mDownloadedCount)
      return position - 1 - (containsOutdated() ? mOutdatedCount + 1 : 0);

    return INVALID_POSITION;
  }

  protected boolean containsOutdated()
  {
    return mOutdatedCount != 0;
  }

  protected boolean containsDownloaded()
  {
    return mDownloadedCount != 0;
  }

  @Override
  protected void updateStatuses()
  {
    notifyDataSetChanged();
  }

  @Override
  public void onCountryProgress(ListView list, MapStorage.Index idx, long current, long total)
  {
    super.onCountryProgress(list, idx, current, total);
  }

  @Override
  public int onCountryStatusChanged(MapStorage.Index idx)
  {
    notifyDataSetChanged();
    return MapStorage.INSTANCE.countryStatus(idx);
  }
}
