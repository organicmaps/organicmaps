package com.mapswithme.maps.downloader;

import android.annotation.SuppressLint;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.text.TextUtils;
import android.util.SparseArray;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Stack;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.widget.WheelProgressView;
import com.mapswithme.util.UiUtils;
import com.timehop.stickyheadersrecyclerview.StickyRecyclerHeadersAdapter;
import com.timehop.stickyheadersrecyclerview.StickyRecyclerHeadersDecoration;

import static com.mapswithme.maps.R.id.status;

class DownloaderAdapter extends RecyclerView.Adapter<DownloaderAdapter.ViewHolder>
                     implements StickyRecyclerHeadersAdapter<DownloaderAdapter.HeaderViewHolder>
{
  private final RecyclerView mRecycler;
  private final StickyRecyclerHeadersDecoration mHeadersDecoration;

  private final List<CountryItem> mItems = new ArrayList<>();
  private final Map<String, CountryItem> mCountryIndex = new HashMap<>();  // Country.id -> Country

  private final SparseArray<String> mHeaders = new SparseArray<>();
  private final Stack<PathEntry> mPath = new Stack<>();

  private int mListenerSlot;

  private static class PathEntry
  {
    final String countryId;
    final int topPosition;
    final int topOffset;

    private PathEntry(String countryId, int topPosition, int topOffset)
    {
      this.countryId = countryId;
      this.topPosition = topPosition;
      this.topOffset = topOffset;
    }
  }

  private final MapManager.StorageCallback mStorageCallback = new MapManager.StorageCallback()
  {
    @Override
    public void onStatusChanged(String countryId, int newStatus)
    {
      CountryItem ci = mCountryIndex.get(countryId);
      if (ci == null)
        return;

      MapManager.nativeGetAttributes(ci);
    }

    @Override
    public void onProgress(String countryId, long localSize, long remoteSize)
    {
      CountryItem ci = mCountryIndex.get(countryId);
      if (ci == null)
        return;

      MapManager.nativeGetAttributes(ci);
    }
  };

  class ViewHolder extends RecyclerView.ViewHolder
  {
    private final WheelProgressView mProgress;
    private final ImageView mStatus;
    private final TextView mName;
    private final TextView mParentName;
    private final TextView mSizes;
    private final TextView mCounts;

    private CountryItem mItem;

    @SuppressLint("WrongViewCast")
    public ViewHolder(View frame)
    {
      super(frame);

      mProgress = (WheelProgressView)frame.findViewById(R.id.progress);
      mStatus = (ImageView)frame.findViewById(status);
      mName = (TextView)frame.findViewById(R.id.name);
      mParentName = (TextView)frame.findViewById(R.id.parent);
      mSizes = (TextView)frame.findViewById(R.id.sizes);
      mCounts = (TextView)frame.findViewById(R.id.counts);

      frame.setOnClickListener(new View.OnClickListener()
      {
        @Override
        public void onClick(View v)
        {
          if (mItem.totalChildCount > 0)
          {
            goDeeper(mItem.id);
            return;
          }

        }
      });

      mStatus.setOnClickListener(new View.OnClickListener()
      {
        @Override
        public void onClick(View v)
        {
          MapManager.nativeGetAttributes(mItem);
        }
      });
    }

    private void updateStatus()
    {
      boolean inProgress = (mItem.status == CountryItem.STATUS_PROGRESS);

      UiUtils.showIf(inProgress, mProgress);
      UiUtils.showIf(!inProgress, mStatus);

      if (inProgress)
      {
        mProgress.setProgress(mItem.progress);
        return;
      }

      switch (mItem.status)
      {
      case CountryItem.STATUS_DONE:
        mStatus.setClickable(false);
        break;

      case CountryItem.STATUS_DOWNLOADABLE:
        mStatus.setClickable(true);
        break;

      case CountryItem.STATUS_FAILED:
        mStatus.setClickable(true);
        break;

      case CountryItem.STATUS_ENQUEUED:
        mStatus.setClickable(false);
        break;

      case CountryItem.STATUS_UPDATABLE:
        mStatus.setImageResource(R.drawable.downloader_update);
        mStatus.setClickable(true);
        break;

      case CountryItem.STATUS_MIXED:
        // TODO (trashkalmar): Status will be replaced with something less senseless
        break;

      default:
        throw new IllegalArgumentException("Inappropriate item status: " + mItem.status);
      }
    }

    void bind(CountryItem item)
    {
      mItem = item;

      mName.setText(mItem.name);
      mParentName.setText(mItem.parentName);
      UiUtils.showIf(!TextUtils.isEmpty(mItem.parentName), mParentName);

      updateStatus();
    }
  }

  class HeaderViewHolder extends RecyclerView.ViewHolder {
    HeaderViewHolder(View frame) {
      super(frame);
    }

    void bind(int position) {
      ((TextView)itemView).setText(mHeaders.get(mItems.get(position).headerId));
    }
  }

  private void collectHeaders()
  {
    mHeaders.clear();

    int headerId = 0;
    int prev = -1;
    for (CountryItem ci: mItems)
    {
      switch (ci.category)
      {
      case CountryItem.CATEGORY_NEAR_ME:
        if (ci.category != prev)
        {
          headerId = CountryItem.CATEGORY_NEAR_ME;
          mHeaders.put(headerId, MwmApplication.get().getString(R.string.downloader_near_me));
          prev = ci.category;
        }
        break;

      case CountryItem.CATEGORY_DOWNLOADED:
        if (ci.category != prev)
        {
          headerId = CountryItem.CATEGORY_DOWNLOADED;
          mHeaders.put(headerId, MwmApplication.get().getString(R.string.downloader_downloaded));
          prev = ci.category;
        }
        break;

      default:
        int prevHeader = headerId;
        headerId = CountryItem.CATEGORY_ALL + ci.name.charAt(0);

        if (headerId != prevHeader)
          mHeaders.put(headerId, ci.name.substring(0, 1).toUpperCase());

        prev = ci.category;
      }

      ci.headerId = headerId;
    }
  }

  private void refreshData()
  {
    mItems.clear();
    MapManager.nativeListItems(getCurrentParent(), mItems);
    Collections.<CountryItem>sort(mItems);
    collectHeaders();

    mCountryIndex.clear();
    for (CountryItem ci: mItems)
      mCountryIndex.put(ci.id, ci);

    mHeadersDecoration.invalidateHeaders();
    //mRecycler.scrollToPosition(0);
    notifyDataSetChanged();
  }

  DownloaderAdapter(RecyclerView recycler)
  {
    mRecycler = recycler;
    mHeadersDecoration = new StickyRecyclerHeadersDecoration(this);
    mRecycler.addItemDecoration(mHeadersDecoration);
    refreshData();
  }

  @Override
  public ViewHolder onCreateViewHolder(ViewGroup parent, int viewType)
  {
    View frame = LayoutInflater.from(parent.getContext()).inflate(R.layout.downloader_item, parent, false);
    return new ViewHolder(frame);
  }

  @Override
  public void onBindViewHolder(ViewHolder holder, int position)
  {
    holder.bind(mItems.get(position));
  }

  @Override
  public HeaderViewHolder onCreateHeaderViewHolder(ViewGroup parent)
  {
    View frame = LayoutInflater.from(parent.getContext()).inflate(R.layout.downloader_item_header, parent, false);
    return new HeaderViewHolder(frame);
  }

  @Override
  public void onBindHeaderViewHolder(HeaderViewHolder holder, int position)
  {
    holder.bind(position);
  }

  @Override
  public long getHeaderId(int position)
  {
    return mItems.get(position).headerId;
  }

  @Override
  public int getItemCount()
  {
    return mItems.size();
  }

  private void goDeeper(String child)
  {
    LinearLayoutManager lm = (LinearLayoutManager)mRecycler.getLayoutManager();
    int position = lm.findFirstVisibleItemPosition();
    int offset = lm.findViewByPosition(position).getTop();

    mPath.push(new PathEntry(child, position, offset));
    refreshData();
  }

  public boolean canGoUpdwards()
  {
    return !mPath.isEmpty();
  }

  public boolean goUpwards()
  {
    if (!canGoUpdwards())
      return false;

    final PathEntry entry = mPath.pop();
    refreshData();

    LinearLayoutManager lm = (LinearLayoutManager)mRecycler.getLayoutManager();
    lm.scrollToPositionWithOffset(entry.topPosition, entry.topOffset);

    return true;
  }

  String getCurrentParent()
  {
    return (canGoUpdwards() ? mPath.peek().countryId : null);
  }

  void attach()
  {
    mListenerSlot = MapManager.nativeSubscribe(mStorageCallback);
  }

  void detach()
  {
    MapManager.nativeUnsubscribe(mListenerSlot);
  }
}
