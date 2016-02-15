package com.mapswithme.maps.downloader;

import android.app.Activity;
import android.support.annotation.AttrRes;
import android.support.annotation.DrawableRes;
import android.support.annotation.Nullable;
import android.support.annotation.StringRes;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.text.TextUtils;
import android.util.SparseArray;
import android.util.SparseIntArray;
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
import com.mapswithme.util.BottomSheetHelper;
import com.mapswithme.util.ThemeUtils;
import com.mapswithme.util.UiUtils;
import com.timehop.stickyheadersrecyclerview.StickyRecyclerHeadersAdapter;
import com.timehop.stickyheadersrecyclerview.StickyRecyclerHeadersDecoration;

import static com.mapswithme.maps.R.id.status;

class DownloaderAdapter extends RecyclerView.Adapter<DownloaderAdapter.ViewHolder>
                     implements StickyRecyclerHeadersAdapter<DownloaderAdapter.HeaderViewHolder>
{
  private final RecyclerView mRecycler;
  private final Activity mActivity;
  private final StickyRecyclerHeadersDecoration mHeadersDecoration;

  private final List<CountryItem> mItems = new ArrayList<>();
  private final Map<String, CountryItem> mCountryIndex = new HashMap<>();  // Country.id -> Country

  private final SparseArray<String> mHeaders = new SparseArray<>();
  private final Stack<PathEntry> mPath = new Stack<>();

  private final SparseIntArray mIconsCache = new SparseIntArray();

  private int mListenerSlot;

  private enum MenuItem
  {
    DELETE(R.drawable.ic_delete, R.string.delete)
    {
      @Override
      void invoke(CountryItem item)
      {
        CANCEL.invoke(item);
        MapManager.nativeDelete(item.id);
      }
    },

    CANCEL(R.drawable.ic_cancel, R.string.cancel)
    {
      @Override
      void invoke(CountryItem item)
      {
        MapManager.nativeCancel(item.id);
      }
    },

    EXPLORE(R.drawable.ic_explore, R.string.zoom_to_country)
    {
      @Override
      void invoke(CountryItem item)
      {

      }
    },

    UPDATE(R.drawable.ic_update, R.string.downloader_update_map)
    {
      @Override
      void invoke(CountryItem item)
      {
        MapManager.nativeGetAttributes(item);

        if (item.status == CountryItem.STATUS_UPDATABLE)
          MapManager.nativeUpdate(item.id);
      }
    };

    final @DrawableRes int icon;
    final @StringRes int title;

    MenuItem(@DrawableRes int icon, @StringRes int title)
    {
      this.icon = icon;
      this.title = title;
    }

    abstract void invoke(CountryItem item);
  }

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
      // TODO
    }

    @Override
    public void onProgress(String countryId, long localSize, long remoteSize)
    {
      CountryItem ci = mCountryIndex.get(countryId);
      if (ci == null)
        return;

      MapManager.nativeGetAttributes(ci);
      // TODO
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

    private void processClick()
    {
      switch (mItem.status)
      {
      case CountryItem.STATUS_DONE:
        processLongClick();
        break;

      case CountryItem.STATUS_DOWNLOADABLE:
        MapManager.nativeDownload(mItem.id);
        break;

      case CountryItem.STATUS_FAILED:
        MapManager.nativeRetry(mItem.id);
        break;

      case CountryItem.STATUS_PROGRESS:
      case CountryItem.STATUS_ENQUEUED:
        MapManager.nativeCancel(mItem.id);
        break;

      case CountryItem.STATUS_UPDATABLE:
        MapManager.nativeUpdate(mItem.id);
        break;

      default:
        throw new IllegalArgumentException("Inappropriate item status: " + mItem.status);
      }
    }

    private void processLongClick()
    {
      List<MenuItem> items = new ArrayList<>();

      switch (mItem.status)
      {
      case CountryItem.STATUS_UPDATABLE:
        items.add(MenuItem.UPDATE);
        // No break

      case CountryItem.STATUS_DONE:
        items.add(MenuItem.EXPLORE);
        items.add(MenuItem.DELETE);
        break;

      case CountryItem.STATUS_FAILED:
        items.add(MenuItem.CANCEL);

        if (mItem.present)
        {
          items.add(MenuItem.DELETE);
          items.add(MenuItem.EXPLORE);
        }
        break;

      case CountryItem.STATUS_PROGRESS:
      case CountryItem.STATUS_ENQUEUED:
        items.add(MenuItem.CANCEL);

        if (mItem.present)
          items.add(MenuItem.EXPLORE);
        break;
      }

      if (items.isEmpty())
        return;

      BottomSheetHelper.Builder bs = BottomSheetHelper.create(mActivity, mItem.name);
      for (MenuItem item: items)
        bs.sheet(item.ordinal(), item.icon, item.title);

      bs.listener(new android.view.MenuItem.OnMenuItemClickListener()
      {
        @Override
        public boolean onMenuItemClick(android.view.MenuItem item)
        {
          MenuItem.values()[item.getItemId()].invoke(mItem);
          return false;
        }
      }).tint().show();
    }

    public ViewHolder(View frame)
    {
      super(frame);

      mProgress = (WheelProgressView) frame.findViewById(R.id.progress);
      mStatus = (ImageView) frame.findViewById(status);
      mName = (TextView) frame.findViewById(R.id.name);
      mParentName = (TextView) frame.findViewById(R.id.parent);
      mSizes = (TextView) frame.findViewById(R.id.sizes);
      mCounts = (TextView) frame.findViewById(R.id.counts);

      frame.setOnClickListener(new View.OnClickListener()
      {
        @Override
        public void onClick(View v)
        {
          if (mItem.isExpandable())
            goDeeper(mItem.id);
          else
            processClick();
        }
      });

      frame.setOnLongClickListener(new View.OnLongClickListener()
      {
        @Override
        public boolean onLongClick(View v)
        {
          processLongClick();
          return true;
        }
      });

      mStatus.setOnClickListener(new View.OnClickListener()
      {
        @Override
        public void onClick(View v)
        {
          processClick();
        }
      });

      mProgress.setOnClickListener(new View.OnClickListener()
      {
        @Override
        public void onClick(View v)
        {
          MapManager.nativeCancel(mItem.id);
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

      boolean clickable = mItem.isExpandable();
      @AttrRes int iconAttr;

      switch (mItem.status)
      {
      case CountryItem.STATUS_DONE:
        clickable = false;
        iconAttr = R.attr.status_done;
        break;

      case CountryItem.STATUS_DOWNLOADABLE:
        iconAttr = R.attr.status_downloadable;
        break;

      case CountryItem.STATUS_FAILED:
        iconAttr = R.attr.status_failed;
        break;

      case CountryItem.STATUS_ENQUEUED:
        clickable = false;
        iconAttr = R.attr.status_updatable;
        break;

      case CountryItem.STATUS_UPDATABLE:
        iconAttr = R.attr.status_updatable;
        break;

      case CountryItem.STATUS_MIXED:
        // TODO (trashkalmar): Status will be replaced with something less senseless
        iconAttr = R.attr.status_updatable;
        break;

      default:
        throw new IllegalArgumentException("Inappropriate item status: " + mItem.status);
      }

      mStatus.setClickable(clickable);
      mStatus.setImageResource(resolveIcon(iconAttr));
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

  private int resolveIcon(@AttrRes int iconAttr)
  {
    int res = mIconsCache.get(iconAttr);
    if (res == 0)
    {
      res = ThemeUtils.getResource(mActivity, R.attr.downloaderTheme, iconAttr);
      mIconsCache.put(iconAttr, res);
    }

    return res;
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
    notifyDataSetChanged();
  }

  DownloaderAdapter(RecyclerView recycler, Activity activity)
  {
    mRecycler = recycler;
    mActivity = activity;
    mHeadersDecoration = new StickyRecyclerHeadersDecoration(this);
    mRecycler.addItemDecoration(mHeadersDecoration);
    refreshData();
  }

  @Override
  public ViewHolder onCreateViewHolder(ViewGroup parent, int viewType)
  {
    return new ViewHolder(LayoutInflater.from(mActivity).inflate(R.layout.downloader_item, parent, false));
  }

  @Override
  public void onBindViewHolder(ViewHolder holder, int position)
  {
    holder.bind(mItems.get(position));
  }

  @Override
  public HeaderViewHolder onCreateHeaderViewHolder(ViewGroup parent)
  {
    return new HeaderViewHolder(LayoutInflater.from(mActivity).inflate(R.layout.downloader_item_header, parent, false));
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

    // Save scroll positions (top item + item`s offset) for current hierarchy level
    int position = lm.findFirstVisibleItemPosition();
    int offset = lm.findViewByPosition(position).getTop();

    mPath.push(new PathEntry(child, position, offset));
    refreshData();
  }

  boolean canGoUpdwards()
  {
    return !mPath.isEmpty();
  }

  boolean goUpwards()
  {
    if (!canGoUpdwards())
      return false;

    final PathEntry entry = mPath.pop();
    refreshData();

    LinearLayoutManager lm = (LinearLayoutManager)mRecycler.getLayoutManager();
    lm.scrollToPositionWithOffset(entry.topPosition, entry.topOffset);

    return true;
  }

  @Nullable String getCurrentParent()
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
