package com.mapswithme.maps.downloader.country;

import android.util.Log;
import android.util.Pair;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ListView;

import com.mapswithme.maps.BuildConfig;
import com.mapswithme.maps.R;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;

@Deprecated
public class OldDownloadedAdapter extends OldBaseDownloadAdapter implements OldActiveCountryTree.ActiveCountryListener
{
  private static final String TAG = OldDownloadedAdapter.class.getSimpleName();
  private static final int TYPE_HEADER = 5;
  private static final int VIEW_TYPE_COUNT = TYPES_COUNT + 1;

  /*
   * Invalid position indicates, that country group(downloaded, new or outdated) and position in that group cannot be determined correctly.
   * Normally we shouldn't fall into that case. However, due to some problems with callbacks it can happen.
   * TODO remove constant & code where it appears after ActiveMapsLayout refactoring.
   */
  private static final int INVALID_POSITION = -1;

  private int mUpdatedCount;
  private int mOutdatedCount;
  private int mInProgressCount;
  private int mListenerSlotId;

  OldDownloadedAdapter(OldDownloadFragment fragment)
  {
    super(fragment);
  }

  @Override
  public View getView(int position, View convertView, ViewGroup parent)
  {
    final int viewType = getItemViewType(position);
    if (viewType == TYPE_HEADER)
    {
      View view = null;
      ViewHolder holder;
      if (convertView == null)
      {
        //view = mInflater.inflate(R.layout.download_item_updated, parent, false);
        holder = new ViewHolder();
        holder.initFromView(view);
        view.setTag(holder);
      }
      else
      {
        view = convertView;
        holder = (ViewHolder) view.getTag();
      }
      if (position == mInProgressCount && containsOutdated())
        holder.mName.setText(mFragment.getString(R.string.downloader_outdated_maps));
      else
        holder.mName.setText(mFragment.getString(R.string.downloader_uptodate_maps));

      return view;
    }
    else
    {
      final View view = super.getView(position, convertView, parent);
      final ViewHolder holder = (ViewHolder) view.getTag();
      UiUtils.showIf(getGroupByAbsPosition(position) == OldActiveCountryTree.GROUP_UP_TO_DATE, holder.mPercent);
      return view;
    }
  }

  @Override
  protected long[] getRemoteItemSizes(int position)
  {
    final Pair<Integer, Integer> groupAndPosition = splitAbsolutePosition(position, "getRemoteItemSizes. Cannot get correct positions.");
    if (groupAndPosition != null)
    {
      long mapSize = OldActiveCountryTree.getCountrySize(groupAndPosition.first, groupAndPosition.second, OldStorageOptions.MAP_OPTION_MAP_ONLY, false);
      long routingSize = OldActiveCountryTree.getCountrySize(groupAndPosition.first, groupAndPosition.second, OldStorageOptions.MAP_OPTION_CAR_ROUTING, false);
      return new long[]{mapSize, mapSize + routingSize};
    }

    return new long[]{0, 0};
  }

  @Override
  protected long[] getDownloadableItemSizes(int position)
  {
    final Pair<Integer, Integer> groupAndPosition = splitAbsolutePosition(position, "getDownloadableItemSizes. Cannot get correct positions.");
    if (groupAndPosition != null)
    {
      long currentSize = OldActiveCountryTree.getCountrySize(groupAndPosition.first, groupAndPosition.second, -1, true);
      long totalSize = OldActiveCountryTree.getCountrySize(groupAndPosition.first, groupAndPosition.second, -1, false);
      return new long[]{currentSize, totalSize};
    }

    return new long[]{0, 0};
  }

  @Override
  public int getCount()
  {
    updateGroupCounters();
    return mInProgressCount + (mUpdatedCount == 0 ? 0 : mUpdatedCount + 1) + (mOutdatedCount == 0 ? 0 : mOutdatedCount + 1);
  }

  private void updateGroupCounters()
  {
    mInProgressCount = OldActiveCountryTree.getCountInGroup(OldActiveCountryTree.GROUP_NEW);
    mUpdatedCount = OldActiveCountryTree.getCountInGroup(OldActiveCountryTree.GROUP_UP_TO_DATE);
    mOutdatedCount = OldActiveCountryTree.getCountInGroup(OldActiveCountryTree.GROUP_OUT_OF_DATE);
  }

  @Override
  public OldCountryItem getItem(int position)
  {
    if (isHeader(position))
      return null;

    final Pair<Integer, Integer> groupAndPosition = splitAbsolutePosition(position, "getItem. Cannot get correct positions.");
    if (groupAndPosition != null)
      return OldActiveCountryTree.getCountryItem(groupAndPosition.first, groupAndPosition.second);

    return OldCountryItem.EMPTY;
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
    return ((containsOutdated() || containsUpdated()) && position == mInProgressCount) ||
        (containsOutdated() && containsUpdated() && position == mInProgressCount + mOutdatedCount + 1);
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
    onCountryClick(getItem(position), view, position);
  }

  private boolean containsOutdated()
  {
    return mOutdatedCount != 0;
  }

  private boolean containsUpdated()
  {
    return mUpdatedCount != 0;
  }

  private int getGroupByAbsPosition(int position)
  {
    final int newGroupEnd = mInProgressCount;
    if (position < newGroupEnd)
      return OldActiveCountryTree.GROUP_NEW;

    final int outdatedGroupEnd = containsOutdated() ? newGroupEnd + mOutdatedCount + 1 : newGroupEnd;
    if (position < outdatedGroupEnd)
      return OldActiveCountryTree.GROUP_OUT_OF_DATE;

    final int updatedGroupEnd = containsUpdated() ? outdatedGroupEnd + mUpdatedCount + 1 : outdatedGroupEnd;
    if (position < updatedGroupEnd)
      return OldActiveCountryTree.GROUP_UP_TO_DATE;

    return INVALID_POSITION;
  }

  private int getPositionInGroup(int position)
  {
    final int newGroupEnd = mInProgressCount;
    if (position < newGroupEnd)
      return position;

    final int outdatedGroupEnd = containsOutdated() ? newGroupEnd + mOutdatedCount + 1 : newGroupEnd;
    if (position < outdatedGroupEnd)
      return position - newGroupEnd - 1;

    final int updatedGroupEnd = containsUpdated() ? outdatedGroupEnd + mUpdatedCount + 1 : outdatedGroupEnd;
    if (position < updatedGroupEnd)
      return position - outdatedGroupEnd - 1;

    return INVALID_POSITION;
  }

  private int getAbsolutePosition(int group, int position)
  {
    switch (group)
    {
    case OldActiveCountryTree.GROUP_NEW:
      return position;
    case OldActiveCountryTree.GROUP_OUT_OF_DATE:
      return position + mInProgressCount + 1;
    default:
      return position + mInProgressCount + 1 + (containsOutdated() ? mOutdatedCount + 1 : 0);
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
  protected void cancelDownload(int position)
  {
    final Pair<Integer, Integer> groupAndPosition = splitAbsolutePosition(position, "cancelDownload. Cannot get correct positions.");
    if (groupAndPosition != null)
      OldActiveCountryTree.cancelDownloading(groupAndPosition.first, groupAndPosition.second);
  }

  @Override
  protected void retryDownload(int position)
  {
    final Pair<Integer, Integer> groupAndPosition = splitAbsolutePosition(position, "retryDownload. Cannot get correct positions.");
    if (groupAndPosition != null)
      OldActiveCountryTree.retryDownloading(groupAndPosition.first, groupAndPosition.second);
  }

  @Override
  public void onResume(ListView listView)
  {
    mListView = listView;
    notifyDataSetChanged();
  }

  @Override
  public void onPause()
  {
    mListView = null;
  }

  @Override
  protected void setCountryListener()
  {
    if (mListenerSlotId == 0)
      mListenerSlotId = OldActiveCountryTree.addListener(this);
  }

  @Override
  protected void resetCountryListener()
  {
    OldActiveCountryTree.removeListener(mListenerSlotId);
    mListenerSlotId = 0;
  }

  @Override
  protected void updateCountry(int position, int options)
  {
    final Pair<Integer, Integer> groupAndPosition = splitAbsolutePosition(position, "updateCountry. Cannot get correct positions.");
    if (groupAndPosition != null)
      OldActiveCountryTree.downloadMap(groupAndPosition.first, groupAndPosition.second, options);
  }

  @Override
  protected void downloadCountry(int position, int options)
  {
    final Pair<Integer, Integer> groupAndPosition = splitAbsolutePosition(position, "downloadCountry. Cannot get correct positions.");
    if (groupAndPosition != null)
      OldActiveCountryTree.downloadMap(groupAndPosition.first, groupAndPosition.second, options);
  }

  @Override
  protected void deleteCountry(int position, int options)
  {
    final Pair<Integer, Integer> groupAndPosition = splitAbsolutePosition(position, "downloadCountry. Cannot get correct positions.");
    if (groupAndPosition != null)
      OldActiveCountryTree.deleteMap(groupAndPosition.first, groupAndPosition.second, options);
  }

  @Override
  protected void showCountry(int position)
  {
    final Pair<Integer, Integer> groupAndPosition = splitAbsolutePosition(position, "showCountry. Cannot get correct positions.");
    if (groupAndPosition != null)
    {
      OldActiveCountryTree.showOnMap(groupAndPosition.first, groupAndPosition.second);
      resetCountryListener();
      Utils.navigateToParent(mFragment.getActivity());
    }
  }

  @Override
  public void onCountryProgressChanged(int group, int position, long[] sizes)
  {
    onCountryProgress(getAbsolutePosition(group, position), sizes[0], sizes[1]);
  }

  @Override
  public void onCountryStatusChanged(int group, int position, int oldStatus, int newStatus)
  {
    updateGroupCounters();
    onCountryStatusChanged(getAbsolutePosition(group, position));
  }

  @Override
  public void onCountryGroupChanged(int oldGroup, int oldPosition, int newGroup, int newPosition)
  {
    notifyDataSetChanged();
  }

  @Override
  public void onCountryOptionsChanged(int group, int position, int newOpt, int requestOpt)
  {
    updateGroupCounters();
    notifyDataSetChanged();
  }

  /**
   * Splits absolute position to pair of (group, position in group)
   *
   * @param position     absolute position
   * @param errorMessage message for the log if split fails
   * @return splitted pair. null if split failed
   */
  private Pair<Integer, Integer> splitAbsolutePosition(int position, String errorMessage)
  {
    final int group = getGroupByAbsPosition(position);
    final int positionInGroup = getPositionInGroup(position);

    if (group == INVALID_POSITION || positionInGroup == INVALID_POSITION)
    {
      handleInvalidPosition(errorMessage, position, group, positionInGroup);
      return null;
    }

    return new Pair<>(group, positionInGroup);
  }

  private void handleInvalidPosition(String message, int position, int group, int positionInGroup)
  {
    Log.d(TAG, message);
    Log.d(TAG, "Current position : " + position);
    Log.d(TAG, "Current group : " + group);
    Log.d(TAG, "Current position in group : " + positionInGroup);
    Log.d(TAG, "InProgress count : " + mInProgressCount);
    Log.d(TAG, "Updated count: " + mUpdatedCount);
    Log.d(TAG, "Outdated count : " + mOutdatedCount);

    if (BuildConfig.DEBUG)
      throw new IllegalStateException(message);
  }
}
