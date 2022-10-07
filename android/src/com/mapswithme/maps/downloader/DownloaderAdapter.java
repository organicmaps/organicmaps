package com.mapswithme.maps.downloader;

import android.app.Activity;
import android.content.Intent;
import android.graphics.Typeface;
import android.location.Location;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.TextUtils;
import android.text.style.StyleSpan;
import android.util.SparseArray;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import androidx.annotation.LayoutRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AlertDialog;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.mapswithme.maps.MwmActivity;
import com.mapswithme.maps.R;
import com.mapswithme.maps.intent.Factory;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.routing.RoutingController;
import com.mapswithme.util.StringUtils;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.bottomsheet.MenuBottomSheetFragment;
import com.mapswithme.util.bottomsheet.MenuBottomSheetItem;
import com.timehop.stickyheadersrecyclerview.StickyRecyclerHeadersAdapter;
import com.timehop.stickyheadersrecyclerview.StickyRecyclerHeadersDecoration;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Stack;

class DownloaderAdapter extends RecyclerView.Adapter<DownloaderAdapter.ViewHolderWrapper>
                     implements StickyRecyclerHeadersAdapter<DownloaderAdapter.HeaderViewHolder>
{
  private static final int HEADER_ADVERTISMENT_ID = CountryItem.CATEGORY__LAST + 1;
  private static final int HEADER_ADS_OFFSET = 10;

  private static final int TYPE_COUNTRY = 0;

  private static final String DOWNLOADER_MENU_ID = "DOWNLOADER_BOTTOM_SHEET";

  private final RecyclerView mRecycler;
  private final Activity mActivity;
  private final DownloaderFragment mFragment;
  private final StickyRecyclerHeadersDecoration mHeadersDecoration;

  private boolean mMyMapsMode = true;
  private boolean mSearchResultsMode;
  private String mSearchQuery;

  private final List<CountryItem> mItems = new ArrayList<>();
  // Core Country.id String -> List<CountryItem> mapping index for updateItem function.
  // Use List, because we have multiple search results now for a single country.
  private final Map<String, List<CountryItem>> mCountryIndex = new HashMap<>();

  private final SparseArray<String> mHeaders = new SparseArray<>();
  private final Stack<PathEntry> mPath = new Stack<>();  // Holds navigation history. The last element is the current level.
  private int mNearMeCount;

  private int mListenerSlot;
  private CountryItem mSelectedItem;

  private void onDownloadActionSelected(final CountryItem item, DownloaderAdapter adapter)
  {
    MapManager.warn3gAndDownload(adapter.mActivity, item.id, null);
  }

  private void onUpdateActionSelected(final CountryItem item, DownloaderAdapter adapter)
  {
    item.update();
    if (item.status != CountryItem.STATUS_UPDATABLE)
      return;
    MapManager.warnOn3gUpdate(adapter.mActivity, item.id, () -> MapManager.nativeUpdate(item.id));
  }

  private void onExploreActionSelected(CountryItem item, DownloaderAdapter adapter)
  {
    Intent intent = new Intent(adapter.mActivity, MwmActivity.class);
    intent.addFlags(Intent.FLAG_ACTIVITY_NO_ANIMATION);
    intent.putExtra(MwmActivity.EXTRA_TASK, new Factory.ShowCountryTask(item.id));
    adapter.mActivity.startActivity(intent);

    if (!(adapter.mActivity instanceof MwmActivity))
      adapter.mActivity.finish();
  }

  void onDeleteActionSelected(final CountryItem item, final DownloaderAdapter adapter)
  {
    if (RoutingController.get().isNavigating())
    {
      new AlertDialog.Builder(adapter.mActivity)
          .setTitle(R.string.downloader_delete_map)
          .setMessage(R.string.downloader_delete_map_while_routing_dialog)
          .setPositiveButton(android.R.string.ok, null)
          .show();
      return;
    }

    if (!MapManager.nativeHasUnsavedEditorChanges(item.id))
    {
      deleteNode(item, adapter);
      return;
    }

    new AlertDialog.Builder(adapter.mActivity)
        .setTitle(R.string.downloader_delete_map)
        .setMessage(R.string.downloader_delete_map_dialog)
        .setNegativeButton(android.R.string.no, null)
        .setPositiveButton(android.R.string.yes, (dialog, which) -> deleteNode(item, adapter)).show();
  }

  private void onCancelActionSelected(CountryItem item, DownloaderAdapter adapter)
  {
    MapManager.nativeCancel(item.id);
  }

  private void deleteNode(CountryItem item)
  {
    MapManager.nativeCancel(item.id);
    MapManager.nativeDelete(item.id);
    OnmapDownloader.setAutodownloadLocked(true);
  }

  private void deleteNode(CountryItem item, DownloaderAdapter adapter)
  {
    if (adapter.mActivity instanceof MwmActivity)
    {
      ((MwmActivity) adapter.mActivity).closePlacePage();
    }
    deleteNode(item);
  }

  private static class PathEntry
  {
    final CountryItem item;
    final boolean myMapsMode;
    final int topPosition;
    final int topOffset;

    private PathEntry(CountryItem item, boolean myMapsMode, int topPosition, int topOffset)
    {
      this.item = item;
      this.myMapsMode = myMapsMode;
      this.topPosition = topPosition;
      this.topOffset = topOffset;
    }

    @Override
    public String toString()
    {
      return item.id + " (" + item.name + "), " +
             "myMapsMode: " + myMapsMode +
             ", topPosition: " + topPosition +
             ", topOffset: " + topOffset;
    }
  }

  private final MapManager.StorageCallback mStorageCallback = new MapManager.StorageCallback()
  {
    private void updateItem(String countryId)
    {
      List<CountryItem> lst = mCountryIndex.get(countryId);
      if (lst == null)
        return;

      for (CountryItem ci : lst) {
        ci.update();

        LinearLayoutManager lm = (LinearLayoutManager) mRecycler.getLayoutManager();
        int first = lm.findFirstVisibleItemPosition();
        int last = lm.findLastVisibleItemPosition();
        if (first == RecyclerView.NO_POSITION || last == RecyclerView.NO_POSITION)
          return;

        for (int i = first; i <= last; i++) {
          ViewHolderWrapper vh = (ViewHolderWrapper) mRecycler.findViewHolderForAdapterPosition(i);
          if (vh != null && vh.mKind == TYPE_COUNTRY && ((CountryItem) vh.mHolder.mItem).id.equals(countryId))
            vh.mHolder.rebind();
        }
      }
    }

    @Override
    public void onStatusChanged(List<MapManager.StorageCallbackData> data)
    {
      for (MapManager.StorageCallbackData item : data)
      {
        if (item.isLeafNode && item.newStatus == CountryItem.STATUS_FAILED)
        {
          MapManager.showError(mActivity, item, null);
          break;
        }
      }

      if (mSearchResultsMode)
      {
        for (MapManager.StorageCallbackData item : data)
          updateItem(item.countryId);
      }
      else
      {
        refreshData();
      }
    }

    @Override
    public void onProgress(String countryId, long localSize, long remoteSize)
    {
      updateItem(countryId);
    }
  };

  private View createViewHolderFrame(ViewGroup parent, int kind)
  {
    return inflate(parent, R.layout.downloader_item);
  }

  class ViewHolderWrapper extends RecyclerView.ViewHolder
  {
    private final int mKind;
    @NonNull
    private final BaseInnerViewHolder mHolder;

    ViewHolderWrapper(@NonNull ViewGroup parent, int kind)
    {
      super(createViewHolderFrame(parent, kind));

      mKind = kind;
      mHolder = new ItemViewHolder(itemView);
    }

    @SuppressWarnings("unchecked")
    void bind(int position)
    {
      mHolder.bind(mItems.get(position));
    }
  }

  private static abstract class BaseInnerViewHolder<T>
  {
    T mItem;

    void bind(T item)
    {
      mItem = item;
    }

    void rebind()
    {
      bind(mItem);
    }
  }

  private void showBottomSheet(CountryItem selectedItem)
  {
    mSelectedItem = selectedItem;
    MenuBottomSheetFragment.newInstance(DOWNLOADER_MENU_ID, mSelectedItem.name)
                           .show(mFragment.getChildFragmentManager(), DOWNLOADER_MENU_ID);
  }

  public ArrayList<MenuBottomSheetItem> getMenuItems()
  {
    ArrayList<MenuBottomSheetItem> items = new ArrayList<>();
    switch (mSelectedItem.status)
    {
      case CountryItem.STATUS_DOWNLOADABLE:
        items.add(getDownloadMenuItem());
        break;

      case CountryItem.STATUS_UPDATABLE:
        items.add(getUpdateMenuItem());
        // Fallthrough

      case CountryItem.STATUS_DONE:
        if (!mSelectedItem.isExpandable())
          items.add(getExploreMenuItem());
        appendDeleteMenuItem(items);
        break;

      case CountryItem.STATUS_FAILED:
        items.add(getCancelMenuItem());

        if (mSelectedItem.present)
        {
          appendDeleteMenuItem(items);
          items.add(getExploreMenuItem());
        }
        break;

      case CountryItem.STATUS_PROGRESS:
      case CountryItem.STATUS_APPLYING:
      case CountryItem.STATUS_ENQUEUED:
        items.add(getCancelMenuItem());

        if (mSelectedItem.present)
          items.add(getExploreMenuItem());
        break;

      case CountryItem.STATUS_PARTLY:
        items.add(getDownloadMenuItem());
        appendDeleteMenuItem(items);
        break;
    }
    return items;
  }


  private MenuBottomSheetItem getDownloadMenuItem()
  {
    return new MenuBottomSheetItem(R.string.downloader_download_map, R.drawable.ic_download,
                                   () -> onDownloadActionSelected(mSelectedItem, DownloaderAdapter.this));
  }

  private MenuBottomSheetItem getUpdateMenuItem()
  {
    return new MenuBottomSheetItem(R.string.downloader_update_map, R.drawable.ic_update,
                                   () -> onUpdateActionSelected(mSelectedItem, DownloaderAdapter.this));
  }

  private MenuBottomSheetItem getExploreMenuItem()
  {
    return new MenuBottomSheetItem(R.string.zoom_to_country, R.drawable.ic_explore,
                                   () -> onExploreActionSelected(mSelectedItem, DownloaderAdapter.this));
  }

  private void appendDeleteMenuItem(ArrayList<MenuBottomSheetItem> items)
  {
    // Do not show "Delete" option for World files.
    // Checking the name is not beautiful, but the simplest way for now ..
    if (!mSelectedItem.id.startsWith("World")) {
      items.add(new MenuBottomSheetItem(R.string.delete, R.drawable.ic_delete,
                                        () -> onDeleteActionSelected(mSelectedItem, DownloaderAdapter.this)));
    }
  }

  private MenuBottomSheetItem getCancelMenuItem()
  {
    return new MenuBottomSheetItem(R.string.cancel, R.drawable.ic_cancel,
                                   () -> onCancelActionSelected(mSelectedItem, DownloaderAdapter.this));
  }

  private class ItemViewHolder extends BaseInnerViewHolder<CountryItem>
  {
    private final DownloaderStatusIcon mStatusIcon;
    private final TextView mName;
    private final TextView mSubtitle;
    private final TextView mFoundName;
    private final TextView mSize;

    private void processClick(boolean clickOnStatus)
    {
      switch (mItem.status)
      {
      case CountryItem.STATUS_DONE:
      case CountryItem.STATUS_PROGRESS:
      case CountryItem.STATUS_APPLYING:
      case CountryItem.STATUS_ENQUEUED:
        processLongClick();
        break;

      case CountryItem.STATUS_DOWNLOADABLE:
      case CountryItem.STATUS_PARTLY:
        if (clickOnStatus)
          onDownloadActionSelected(mItem, DownloaderAdapter.this);
        else
          processLongClick();
        break;

      case CountryItem.STATUS_FAILED:
        RetryFailedDownloadConfirmationListener listener =
            new RetryFailedDownloadConfirmationListener(mActivity.getApplication());
        MapManager.warn3gAndRetry(mActivity, mItem.id, listener);
        break;

      case CountryItem.STATUS_UPDATABLE:
        MapManager.warnOn3gUpdate(mActivity, mItem.id, new Runnable()
        {
          @Override
          public void run()
          {
            MapManager.nativeUpdate(mItem.id);
          }
        });
        break;

      default:
        throw new IllegalArgumentException("Inappropriate item status: " + mItem.status);
      }
    }

    private void processLongClick()
    {
      showBottomSheet(mItem);
    }

    ItemViewHolder(View frame)
    {
      mStatusIcon = new DownloaderStatusIcon(frame.findViewById(R.id.downloader_status_frame))
      {
        @Override
        protected int selectIcon(CountryItem country)
        {
          if (country.status == CountryItem.STATUS_DOWNLOADABLE || country.status == CountryItem.STATUS_PARTLY)
          {
            return (country.isExpandable() ? (mMyMapsMode ? R.attr.status_folder_done
                                                          : R.attr.status_folder)
                                           : R.attr.status_downloadable);
          }

          return super.selectIcon(country);
        }

        @Override
        protected void updateIcon(CountryItem country)
        {
          super.updateIcon(country);
          mIcon.setFocusable(country.isExpandable() && country.status != CountryItem.STATUS_DONE);
        }
      }.setOnIconClickListener(new View.OnClickListener()
      {
        @Override
        public void onClick(View v)
        {
          processClick(true);
        }
      }).setOnCancelClickListener(new View.OnClickListener()
      {
        @Override
        public void onClick(View v)
        {
          MapManager.nativeCancel(mItem.id);
        }
      });

      mName = frame.findViewById(R.id.name);
      mSubtitle = frame.findViewById(R.id.subtitle);
      mFoundName = frame.findViewById(R.id.found_name);
      mSize = frame.findViewById(R.id.size);

      frame.setOnClickListener(new View.OnClickListener()
      {
        @Override
        public void onClick(View v)
        {
          if (mItem.isExpandable())
            goDeeper(mItem, true);
          else
            processClick(false);
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
    }

    @Override
    void bind(CountryItem item)
    {
      super.bind(item);

      String found = null;
      if (mSearchResultsMode)
      {
        mName.setMaxLines(1);
        mName.setText(mItem.name);

        String searchResultName = mItem.searchResultName;
        if (!TextUtils.isEmpty(searchResultName))
        {
          found = searchResultName.toLowerCase();
          SpannableStringBuilder builder = new SpannableStringBuilder(searchResultName);
          int start = found.indexOf(mSearchQuery);
          int end = start + mSearchQuery.length();
          if (start > -1)
            builder.setSpan(new StyleSpan(Typeface.BOLD), start, end, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);

          mFoundName.setText(builder);
        }

        if (!mItem.isExpandable())
          UiUtils.setTextAndHideIfEmpty(mSubtitle, mItem.topmostParentName);
      }
      else
      {
        mName.setMaxLines(2);
        mName.setText(mItem.name);
        if (!mItem.isExpandable())
          UiUtils.setTextAndHideIfEmpty(mSubtitle, mItem.description);
      }

      if (mItem.isExpandable())
      {
        UiUtils.setTextAndHideIfEmpty(mSubtitle, String.format("%s: %s", mActivity.getString(R.string.downloader_status_maps),
                                                                         mActivity.getString(R.string.downloader_of, mItem.childCount,
                                                                                                                     mItem.totalChildCount)));
      }

      UiUtils.showIf(mSearchResultsMode && !TextUtils.isEmpty(found), mFoundName);

      long size;
      if (mItem.status == CountryItem.STATUS_ENQUEUED ||
          mItem.status == CountryItem.STATUS_PROGRESS ||
          mItem.status == CountryItem.STATUS_APPLYING)
      {
        size = mItem.enqueuedSize;
      }
      else
      {
        size = ((!mSearchResultsMode && mMyMapsMode) ? mItem.size : mItem.totalSize);
      }

      mSize.setText(StringUtils.getFileSizeString(mFragment.requireContext(), size));
      mStatusIcon.update(mItem);
    }
  }

  class HeaderViewHolder extends RecyclerView.ViewHolder
  {
    @NonNull
    private final TextView mTitle;

    HeaderViewHolder(@NonNull View frame)
    {
      super(frame);
      mTitle = frame.findViewById(R.id.title);
    }

    void bind(int position)
    {
        CountryItem ci = mItems.get(position);
        mTitle.setText(mHeaders.get(ci.headerId));
    }
  }

  private void collectHeaders()
  {
    mNearMeCount = 0;
    mHeaders.clear();
    if (mSearchResultsMode)
      return;

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
          mHeaders.put(headerId, mActivity.getString(R.string.downloader_near_me_subtitle));
          prev = ci.category;
        }

        mNearMeCount++;
        break;

      case CountryItem.CATEGORY_DOWNLOADED:
        if (ci.category != prev)
        {
          headerId = CountryItem.CATEGORY_DOWNLOADED;
          mHeaders.put(headerId, mActivity.getString(R.string.downloader_downloaded_subtitle));
          prev = ci.category;
        }
        break;

      default:
        int prevHeader = headerId;
        headerId = CountryItem.CATEGORY_AVAILABLE * HEADER_ADS_OFFSET + ci.name.charAt(0);

        if (headerId != prevHeader)
          mHeaders.put(headerId, ci.name.substring(0, 1).toUpperCase());

        prev = ci.category;
      }

      ci.headerId = headerId;
    }
  }

  void refreshData()
  {
    mItems.clear();

    String parent = getCurrentRootId();
    boolean hasLocation = false;
    double lat = 0.0;
    double lon = 0.0;

    if (!mMyMapsMode && CountryItem.isRoot(parent))
    {
      Location loc = LocationHelper.INSTANCE.getSavedLocation();
      hasLocation = (loc != null);
      if (hasLocation)
      {
        lat = loc.getLatitude();
        lon = loc.getLongitude();
      }
    }

    MapManager.nativeListItems(parent, lat, lon, hasLocation, mMyMapsMode, mItems);
    processData();
  }

  void setSearchResultsMode(@NonNull Collection<CountryItem> results, String query)
  {
    mSearchResultsMode = true;
    mSearchQuery = query.toLowerCase();

    mItems.clear();
    mItems.addAll(results);
    processData();
  }

  void resetSearchResultsMode()
  {
    mSearchResultsMode = false;
    mSearchQuery = null;
    refreshData();
  }

  private void processData()
  {
    if (!mSearchResultsMode)
      Collections.sort(mItems);

    collectHeaders();

    mCountryIndex.clear();
    for (CountryItem ci: mItems) {
      List<CountryItem> lst = mCountryIndex.get(ci.id);
      if (lst != null)
        lst.add(ci);
      else {
        lst = new ArrayList<>();
        lst.add(ci);
        mCountryIndex.put(ci.id, lst);
      }
    }

    if (mItems.isEmpty())
      mFragment.setupPlaceholder();

    mFragment.showPlaceholder(mItems.isEmpty());

    mHeadersDecoration.invalidateHeaders();
    notifyDataSetChanged();
  }

  DownloaderAdapter(DownloaderFragment fragment)
  {
    mActivity = fragment.requireActivity();
    mFragment = fragment;
    mRecycler = mFragment.getRecyclerView();
    mHeadersDecoration = new StickyRecyclerHeadersDecoration(this);
    mRecycler.addItemDecoration(mHeadersDecoration);
  }

  @NonNull
  private View inflate(ViewGroup parent, @LayoutRes int layoutId)
  {
    return LayoutInflater.from(mActivity).inflate(layoutId, parent, false);
  }

  @Override
  public int getItemViewType(int position)
  {
    return TYPE_COUNTRY;
  }

  @Override
  public ViewHolderWrapper onCreateViewHolder(ViewGroup parent, int viewType)
  {
    return new ViewHolderWrapper(parent, viewType);
  }

  @Override
  public void onBindViewHolder(ViewHolderWrapper holder, int position)
  {
    holder.bind(position);
  }

  @Override
  public HeaderViewHolder onCreateHeaderViewHolder(ViewGroup parent)
  {
    return new HeaderViewHolder(inflate(parent, R.layout.downloader_item_header));
  }

  @Override
  public void onBindHeaderViewHolder(HeaderViewHolder holder, int position)
  {
    holder.bind(position);
  }

  @Override
  public long getHeaderId(int position)
  {
    if (position >= mNearMeCount)
    {
      if (position < mNearMeCount)
        return HEADER_ADVERTISMENT_ID;
    }

    return mItems.get(position).headerId;
  }

  @Override
  public int getItemCount()
  {
    return mItems.size();
  }

  private void goDeeper(CountryItem child, boolean refresh)
  {
    LinearLayoutManager lm = (LinearLayoutManager)mRecycler.getLayoutManager();

    // Save scroll positions (top item + item`s offset) for current hierarchy level
    int position = lm.findFirstVisibleItemPosition();
    int offset;

    if (position > -1)
      offset = lm.findViewByPosition(position).getTop();
    else
    {
      position = 0;
      offset = 0;
    }

    boolean wasEmpty = mPath.isEmpty();
    mPath.push(new PathEntry(child, mMyMapsMode, position, offset));
    mMyMapsMode &= (!mSearchResultsMode || child.childCount > 0);

    if (wasEmpty)
      mFragment.clearSearchQuery();

    lm.scrollToPosition(0);

    if (!refresh)
      return;

    if (mSearchResultsMode)
      resetSearchResultsMode();
    else
      refreshData();
    mFragment.update();
  }

  boolean canGoUpwards()
  {
    return !mPath.isEmpty();
  }

  boolean goUpwards()
  {
    if (!canGoUpwards())
      return false;

    PathEntry entry = mPath.pop();
    mMyMapsMode = entry.myMapsMode;
    refreshData();

    LinearLayoutManager lm = (LinearLayoutManager) mRecycler.getLayoutManager();
    lm.scrollToPositionWithOffset(entry.topPosition, entry.topOffset);

    mFragment.update();
    return true;
  }

  void setAvailableMapsMode()
  {
    goDeeper(getCurrentRootItem(), false);
    mMyMapsMode = false;
    refreshData();
  }

  private CountryItem getCurrentRootItem()
  {
    return (canGoUpwards() ? mPath.peek().item : CountryItem.fill(CountryItem.getRootId()));
  }

  @NonNull String getCurrentRootId()
  {
    return (canGoUpwards() ? getCurrentRootItem().id : CountryItem.getRootId());
  }

  @Nullable String getCurrentRootName()
  {
    return (canGoUpwards() ? getCurrentRootItem().name : null);
  }

  boolean isMyMapsMode()
  {
    return mMyMapsMode;
  }

  void attach()
  {
    mListenerSlot = MapManager.nativeSubscribe(mStorageCallback);
  }

  void detach()
  {
    MapManager.nativeUnsubscribe(mListenerSlot);
  }

  boolean isSearchResultsMode()
  {
    return mSearchResultsMode;
  }
}
