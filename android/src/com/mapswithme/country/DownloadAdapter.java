package com.mapswithme.country;

import android.database.DataSetObserver;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.util.Utils;

@Deprecated
class DownloadAdapter extends BaseDownloadAdapter implements CountryTree.CountryTreeListener
{
  private static final int EXTENDED_VIEWS_COUNT = 2; // 3 more views at the top of baseadapter
  private static final int DOWNLOADED_ITEM_POSITION = 0;
  private static final int EXTENDED_VIEW_TYPE_COUNT = 2; // one for placeholder, other for downloader view
  private static final int TYPE_PLACEHOLDER = 5;
  static final int TYPE_EXTENDED = 6;

  private int mLastDownloadedCount = -1;

  private static class ViewHolder extends BaseDownloadAdapter.ViewHolder
  {
    TextView tvCount;

    @Override
    void initFromView(View v)
    {
      super.initFromView(v);
      tvCount = (TextView) v.findViewById(R.id.tv__count);
    }
  }

  @Override
  protected int adjustPosition(int position)
  {
    if (hasHeaderItems())
      return position - EXTENDED_VIEWS_COUNT;

    return position;
  }

  @Override
  protected int unadjustPosition(int position)
  {
    if (hasHeaderItems())
      return position + EXTENDED_VIEWS_COUNT;

    return position;
  }

  DownloadAdapter(DownloadFragment fragment)
  {
    super(fragment);
    CountryTree.setDefaultRoot();

    mFragment.getDownloadedAdapter().registerDataSetObserver(new DataSetObserver()
    {
      @Override
      public void onChanged()
      {
        onDownloadedCountChanged(mFragment.getDownloadedAdapter().getCount());
      }
    });
  }

  @Override
  public boolean isEnabled(int position)
  {
    if (hasHeaderItems())
    {
      if (position == DOWNLOADED_ITEM_POSITION)
        return true;

      if (position < EXTENDED_VIEWS_COUNT)
        return false;
    }

    return super.isEnabled(position);
  }

  @Override
  public int getViewTypeCount()
  {
    return super.getViewTypeCount() + EXTENDED_VIEW_TYPE_COUNT;
  }

  @Override
  public int getItemViewType(int position)
  {
    if (hasHeaderItems())
    {
      if (position == DOWNLOADED_ITEM_POSITION)
        return TYPE_EXTENDED;

      if (position < EXTENDED_VIEWS_COUNT)
        return TYPE_PLACEHOLDER;
    }

    return super.getItemViewType(position);
  }

  @Override
  public View getView(int position, View convertView, ViewGroup parent)
  {
    final int viewType = getItemViewType(position);
    ViewHolder holder = new ViewHolder();
    if (viewType == TYPE_PLACEHOLDER)
    {
      final View view = getPlaceholderView(parent);
      holder.initFromView(view);
      view.setTag(holder);
      return view;
    }

    if (viewType == TYPE_EXTENDED)
    {
      final View view = getExtendedView(parent);
      holder.initFromView(view);
      final int count = ActiveCountryTree.getOutOfDateCount();
      if (count > 0)
      {
        holder.tvCount.setVisibility(View.VISIBLE);
        holder.tvCount.setText(String.valueOf(count));
      }
      else
        holder.tvCount.setVisibility(View.GONE);
      view.setTag(holder);
      return view;
    }

    return super.getView(position, convertView, parent);
  }

  private View getPlaceholderView(ViewGroup parent)
  {
    return mInflater.inflate(R.layout.download_item_placeholder, parent, false);
  }

  private View getExtendedView(ViewGroup parent)
  {
    return mInflater.inflate(R.layout.download_item_extended, parent, false);
  }

  private void onDownloadedCountChanged(int newCount)
  {
    if (mLastDownloadedCount != newCount)
    {
      mLastDownloadedCount = newCount;
      notifyDataSetChanged();
    }
  }

  @Override
  protected boolean hasHeaderItems()
  {
    if (mLastDownloadedCount == -1)
      mLastDownloadedCount = mFragment.getDownloadedAdapter().getCount();

    return !CountryTree.hasParent() && (mLastDownloadedCount > 0);
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
      expandGroup(adjustPosition(position));
    else
      onCountryClick(item, view, position);
  }

  @Override
  public int getCount()
  {
    int res = CountryTree.getChildCount();

    if (hasHeaderItems())
      res += EXTENDED_VIEWS_COUNT;

    return res;
  }

  @Override
  public CountryItem getItem(int position)
  {
    return CountryTree.getChildItem(adjustPosition(position));
  }

  @Override
  protected void showCountry(int position)
  {
    CountryTree.showLeafOnMap(position);
    resetCountryListener();
    Utils.navigateToParent(mFragment.getActivity());
  }

  @Override
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
  protected void cancelDownload(int position)
  {
    CountryTree.cancelDownloading(position);
  }

  @Override
  protected void retryDownload(int position)
  {
    CountryTree.retryDownloading(position);
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
    onCountryStatusChanged(unadjustPosition(position));
  }
}
