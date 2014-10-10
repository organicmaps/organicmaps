package com.mapswithme.country;

import android.app.Activity;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.R;
import com.mapswithme.maps.guides.GuideInfo;

public class DownloadedAdapter extends BaseDownloadAdapter implements ActiveCountryTree.ActiveCountryListener
{
  private static final int TYPE_HEADER = 5;
  private static final int VIEW_TYPE_COUNT = 6;
  private int mUpdatedCount;
  private int mOutdatedCount;
  private int mInProgressCount;
  private int mListenerSlotId;

  public DownloadedAdapter(Activity activity)
  {
    super(activity);
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
      if (position == mInProgressCount)
        holder.mName.setText(mActivity.getString(R.string.downloader_outdated_maps));
      else
        holder.mName.setText(mActivity.getString(R.string.downloader_uptodate_maps));

      return view;
    }
    else
    {
      final View view = super.getView(position, convertView, parent);
      final ViewHolder holder = (ViewHolder) view.getTag();
      if (getGroupByAbsPosition(position) != ActiveCountryTree.GROUP_UP_TO_DATE)
        holder.mPercent.setVisibility(View.GONE);
      else
        holder.mPercent.setVisibility(View.VISIBLE);
      return view;
    }
  }

  @Override
  protected long[] getItemSizes(int position, int options)
  {
    return ActiveCountryTree.getCountrySize(getGroupByAbsPosition(position), getPositionInGroup(position), options);
  }

  @Override
  protected long[] getDownloadableItemSizes(int position)
  {
    return ActiveCountryTree.getDownloadableCountrySize(getGroupByAbsPosition(position), getPositionInGroup(position));
  }

  @Override
  public int getCount()
  {
    mInProgressCount = ActiveCountryTree.getCountInGroup(ActiveCountryTree.GROUP_NEW);
    mUpdatedCount = ActiveCountryTree.getCountInGroup(ActiveCountryTree.GROUP_UP_TO_DATE);
    mOutdatedCount = ActiveCountryTree.getCountInGroup(ActiveCountryTree.GROUP_OUT_OF_DATE);
    return mInProgressCount + (mUpdatedCount == 0 ? 0 : mUpdatedCount + 1) + (mOutdatedCount == 0 ? 0 : mOutdatedCount + 1);
  }

  @Override
  public CountryItem getItem(int position)
  {
    if (isHeader(position))
      return null;

    final int group = getGroupByAbsPosition(position);
    final int positionInGroup = getPositionInGroup(position);
    return new CountryItem(ActiveCountryTree.getCountryName(group, positionInGroup),
        ActiveCountryTree.getCountryStatus(group, positionInGroup),
        ActiveCountryTree.getCountryOptions(group, positionInGroup),
        false);
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
    return (containsOutdated() && position == mInProgressCount) ||
        (!containsOutdated() && containsDownloaded() && position == mInProgressCount) ||
        (containsOutdated() && containsDownloaded() && position == mInProgressCount + mOutdatedCount + 1);
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
  public void onItemClick(int position, View view)
  {
    showCountryContextMenu(getItem(position), view, position);
  }

  protected boolean containsOutdated()
  {
    return mOutdatedCount != 0;
  }

  protected boolean containsDownloaded()
  {
    return mUpdatedCount != 0;
  }

  protected boolean containsInProgress()
  {
    return mInProgressCount != 0;
  }

  @Override
  protected void fillList()
  {
    notifyDataSetChanged();
  }

  private int getGroupByAbsPosition(int position)
  {
    final int newGroupEnd = mInProgressCount;
    if (position < newGroupEnd)
      return ActiveCountryTree.GROUP_NEW;

    final int outdatedGroupEnd = newGroupEnd + mOutdatedCount + 1;
    if (position < outdatedGroupEnd)
      return ActiveCountryTree.GROUP_OUT_OF_DATE;

    final int updatedGroupEnd = outdatedGroupEnd + mUpdatedCount + 1;
    if (position < updatedGroupEnd)
      return ActiveCountryTree.GROUP_UP_TO_DATE;

    return INVALID_POSITION;
  }

  private int getPositionInGroup(int position)
  {
    final int newGroupEnd = mInProgressCount;
    if (position < newGroupEnd)
      return position;

    final int outdatedGroupEnd = newGroupEnd + mOutdatedCount + 1;
    if (position < outdatedGroupEnd)
      return position - newGroupEnd - 1;

    final int updatedGroupEnd = outdatedGroupEnd + mUpdatedCount + 1;
    if (position < updatedGroupEnd)
      return position - outdatedGroupEnd - 1;

    return INVALID_POSITION;
  }

  private int getAbsolutePosition(int group, int position)
  {
    switch (group)
    {
    case ActiveCountryTree.GROUP_NEW:
      return position;
    case ActiveCountryTree.GROUP_OUT_OF_DATE:
      return position + mInProgressCount + 1;
    default:
      return position + mInProgressCount + mOutdatedCount + 2;
    }
  }

  @Override
  protected void expandGroup(int position)
  {
    // no groups in current design
  }

  @Override
  public boolean onBackPressed()
  {
    return false;
  }

  @Override
  protected GuideInfo getGuideInfo(int position)
  {
    return ActiveCountryTree.getGuideInfo(getGroupByAbsPosition(position), getPositionInGroup(position));
  }

  @Override
  protected void cancelDownload(int position)
  {
    ActiveCountryTree.cancelDownloading(getGroupByAbsPosition(position), getPositionInGroup(position));
  }

  @Override
  protected void setCountryListener()
  {
    mListenerSlotId = ActiveCountryTree.addListener(this);
  }

  @Override
  protected void resetCountryListener()
  {
    ActiveCountryTree.removeListener(mListenerSlotId);
  }

  @Override
  protected void updateCountry(int position, int options)
  {
    ActiveCountryTree.downloadMap(getGroupByAbsPosition(position), getPositionInGroup(position), options);
  }

  @Override
  protected void downloadCountry(int position, int options)
  {
    ActiveCountryTree.downloadMap(getGroupByAbsPosition(position), getPositionInGroup(position), options);
  }

  @Override
  protected void deleteCountry(int position, int options)
  {
    ActiveCountryTree.deleteMap(getGroupByAbsPosition(position), getPositionInGroup(position), options);
  }

  @Override
  protected void showCountry(int position)
  {
    ActiveCountryTree.showOnMap(getGroupByAbsPosition(position), getPositionInGroup(position));
    resetCountryListener();
    mActivity.finish();
  }

  @Override
  public void onCountryProgressChanged(int group, int position, long[] sizes)
  {
    onCountryProgress(getAbsolutePosition(group, position), sizes[0], sizes[1]);
  }

  @Override
  public void onCountryStatusChanged(int group, int position)
  {
    onCountryStatusChanged(getAbsolutePosition(group, position));
  }

  @Override
  public void onCountryGroupChanged(int oldGroup, int oldPosition, int newGroup, int newPosition)
  {
    notifyDataSetChanged();
  }

  @Override
  public void onCountryOptionsChanged(int group, int position)
  {
    notifyDataSetChanged();
  }
}
