package app.organicmaps.downloader;

import android.app.Activity;
import android.content.Intent;
import android.graphics.Typeface;
import android.location.Location;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.TextUtils;
import android.text.style.StyleSpan;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;
import androidx.activity.OnBackPressedCallback;
import androidx.annotation.LayoutRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import app.organicmaps.MwmActivity;
import app.organicmaps.MwmApplication;
import app.organicmaps.R;
import app.organicmaps.sdk.downloader.CountryItem;
import app.organicmaps.sdk.downloader.MapManager;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.util.StringUtils;
import app.organicmaps.util.UiUtils;
import app.organicmaps.widget.WheelProgressView;
import app.organicmaps.util.bottomsheet.MenuBottomSheetFragment;
import app.organicmaps.util.bottomsheet.MenuBottomSheetItem;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Stack;

class DownloaderAdapter extends RecyclerView.Adapter<DownloaderAdapter.ViewHolderWrapper>
{
  private static final int TYPE_COUNTRY = 0;
  private static final int TYPE_HEADER = 1;
  private static final int TYPE_TERRAIN = 2;

  private static final String DOWNLOADER_MENU_ID = "DOWNLOADER_BOTTOM_SHEET";

  private final RecyclerView mRecycler;
  private final Activity mActivity;
  private final DownloaderFragment mFragment;
  private boolean mMyMapsMode = true;
  private boolean mSearchResultsMode;
  private String mSearchQuery;

  private final List<GenericItem> mItemsAndHeader = new ArrayList<>();

  private final List<CountryItem> mItems = new ArrayList<>();
  // Core Country.id String -> List<CountryItem> mapping index for updateItem function.
  // Use List, because we have multiple search results now for a single country.
  private final Map<String, List<CountryItem>> mCountryIndex = new HashMap<>();

  private final Stack<PathEntry> mPath =
      new Stack<>(); // Holds navigation history. The last element is the current level.

  private int mListenerSlot;
  private CountryItem mSelectedItem;

  private final OnBackPressedCallback mBackPressedCallback = new OnBackPressedCallback(false) {
    @Override
    public void handleOnBackPressed()
    {
      goUpwards();
      setEnabled(canGoUpwards());
    }
  };

  private static class GenericItem
  {
    @Nullable
    public final String mHeaderText;
    @Nullable
    public final CountryItem mItem;
    // True for the synthetic terrain (.twm) sub-row of mItem's region.
    public final boolean mIsTerrain;
    public GenericItem(@Nullable CountryItem item)
    {
      this(item, false);
    }
    public GenericItem(@Nullable CountryItem item, boolean isTerrain)
    {
      mItem = item;
      mHeaderText = null;
      mIsTerrain = isTerrain;
    }
    public GenericItem(@Nullable String headerText)
    {
      mItem = null;
      mHeaderText = headerText;
      mIsTerrain = false;
    }
  }

  private void onDownloadActionSelected(final CountryItem item, DownloaderAdapter adapter)
  {
    MapManagerHelper.warn3gAndDownload(adapter.mActivity, item.id, null);
  }

  private void onUpdateActionSelected(final CountryItem item, DownloaderAdapter adapter)
  {
    item.update();
    if (item.status != CountryItem.STATUS_UPDATABLE)
      return;
    MapManagerHelper.warnOn3gUpdate(adapter.mActivity, item.id,
                                    () -> MapManagerHelper.startUpdate(adapter.mActivity, item.id));
  }

  private void onExploreActionSelected(CountryItem item, DownloaderAdapter adapter)
  {
    Intent intent = new Intent(adapter.mActivity, MwmActivity.class);
    intent.addFlags(Intent.FLAG_ACTIVITY_NO_ANIMATION);
    intent.putExtra(MwmActivity.EXTRA_COUNTRY_ID, item.id);
    adapter.mActivity.startActivity(intent);

    if (!(adapter.mActivity instanceof MwmActivity))
      adapter.mActivity.finish();
  }

  void onDeleteActionSelected(final CountryItem item, final DownloaderAdapter adapter)
  {
    if (RoutingController.get().isNavigating())
    {
      new MaterialAlertDialogBuilder(adapter.mActivity, R.style.MwmTheme_AlertDialog)
          .setTitle(R.string.downloader_delete_map)
          .setMessage(R.string.downloader_delete_map_while_routing_dialog)
          .setPositiveButton(R.string.ok, null)
          .show();
      return;
    }

    if (!MapManager.nativeHasUnsavedEditorChanges(item.id))
    {
      deleteNode(item, adapter);
      return;
    }

    new MaterialAlertDialogBuilder(adapter.mActivity, R.style.MwmTheme_AlertDialog)
        .setTitle(R.string.downloader_delete_map)
        .setMessage(R.string.downloader_delete_map_dialog)
        .setNegativeButton(R.string.cancel, null)
        .setPositiveButton(R.string.ok, (dialog, which) -> deleteNode(item, adapter))
        .show();
  }

  private void onCancelActionSelected(CountryItem item)
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
    refreshData();
  }

  private record PathEntry(CountryItem item, boolean myMapsMode, int topPosition, int topOffset)
  {
    @Override
    public String toString()
    {
      return item.id + " (" + item.name + "), "
    + "myMapsMode: " + myMapsMode + ", topPosition: " + topPosition + ", topOffset: " + topOffset;
    }
  }

  private final MapManager.StorageCallback mStorageCallback = new MapManager.StorageCallback() {
    private void updateItem(String countryId)
    {
      List<CountryItem> lst = mCountryIndex.get(countryId);
      if (lst == null)
        return;

      for (CountryItem ci : lst)
      {
        ci.update();

        LinearLayoutManager lm = (LinearLayoutManager) mRecycler.getLayoutManager();
        int first = lm.findFirstVisibleItemPosition();
        int last = lm.findLastVisibleItemPosition();
        if (first == RecyclerView.NO_POSITION || last == RecyclerView.NO_POSITION)
          return;

        for (int i = first; i <= last; i++)
        {
          ViewHolderWrapper vh = (ViewHolderWrapper) mRecycler.findViewHolderForAdapterPosition(i);
          if (vh != null && (vh.mKind == TYPE_COUNTRY || vh.mKind == TYPE_TERRAIN)
              && ((CountryItem) vh.mHolder.mItem).id.equals(countryId))
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
          MapManagerHelper.showError(mActivity, item, null);
          break;
        }
      }

      for (MapManager.StorageCallbackData item : data)
      {
        updateItem(item.countryId);
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
    if (kind == TYPE_COUNTRY || kind == TYPE_TERRAIN)
      return inflate(parent, R.layout.downloader_item);
    else
      return inflate(parent, R.layout.downloader_item_header);
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
      if (mKind == TYPE_COUNTRY)
        mHolder = new ItemViewHolder(itemView);
      else if (mKind == TYPE_TERRAIN)
        mHolder = new TerrainViewHolder(itemView);
      else
        mHolder = new HeaderViewHolder(itemView);
    }

    @SuppressWarnings("unchecked")
    void bind(int position)
    {
      final GenericItem item = mItemsAndHeader.get(position);
      if ((mKind == TYPE_COUNTRY || mKind == TYPE_TERRAIN) && item.mItem != null)
        mHolder.bind(item.mItem);
      else if (item.mHeaderText != null)
        mHolder.bind(item.mHeaderText);
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

  // The synthetic terrain (.twm) sub-row of a region: status + size + progress; a tap
  // starts the terrain downloading. Reuses the downloader_item layout.
  // TODO(terrain): move the titles into data/strings/strings.txt before shipping.
  private class TerrainViewHolder extends BaseInnerViewHolder<CountryItem>
  {
    static final int TERRAIN_NOT_AVAILABLE = 0;
    static final int TERRAIN_NOT_DOWNLOADED = 1;
    static final int TERRAIN_DOWNLOADING = 2;
    static final int TERRAIN_PARTLY = 3;
    static final int TERRAIN_ON_DISK = 4;
    static final int TERRAIN_FAILED = 5;

    private final WheelProgressView mProgress;
    private final ImageView mStatus;
    private final TextView mName;
    private final TextView mSubtitle;
    private final TextView mSize;

    TerrainViewHolder(View frame)
    {
      mProgress = frame.findViewById(R.id.downloader_progress_wheel);
      mStatus = frame.findViewById(R.id.downloader_status);
      mName = frame.findViewById(R.id.name);
      mSubtitle = frame.findViewById(R.id.subtitle);
      mSize = frame.findViewById(R.id.size);
      frame.findViewById(R.id.found_name).setVisibility(View.GONE);

      // The status icon is the download button (like the map rows); the whole row
      // triggers the same action for a bigger tap target.
      final View.OnClickListener clickListener = v ->
      {
        if (mItem == null)
          return;
        if (mItem.terrainStatus == TERRAIN_NOT_DOWNLOADED || mItem.terrainStatus == TERRAIN_PARTLY
            || mItem.terrainStatus == TERRAIN_FAILED)
        {
          // The same flow as the maps: the cellular warning, then the foreground
          // service keeps the shared download queue alive in the background.
          final String id = mItem.id;
          final long remaining = mItem.terrainTotalSize - mItem.terrainDownloadedBytes;
          MapManagerHelper.warnOn3g(mActivity, remaining,
                                    () -> DownloaderService.startTerrainDownload(mActivity, id));
        }
      };
      frame.setOnClickListener(clickListener);
      frame.findViewById(R.id.downloader_status_frame).setOnClickListener(clickListener);
    }

    @Override
    void bind(CountryItem item)
    {
      super.bind(item);

      mName.setText("Terrain");
      mSize.setText(StringUtils.getFileSizeString(mSize.getContext(), item.terrainTotalSize));

      boolean downloading = item.terrainStatus == TERRAIN_DOWNLOADING;
      UiUtils.showIf(downloading, mProgress);
      UiUtils.showIf(!downloading, mStatus);
      if (downloading)
      {
        int percent = item.terrainTotalSize > 0
                        ? (int) (item.terrainDownloadedBytes * 100 / item.terrainTotalSize) : 0;
        mProgress.setPending(false);
        mProgress.setProgress(percent);
        mSubtitle.setText(StringUtils.getFileSizeString(mSubtitle.getContext(), item.terrainDownloadedBytes) + " / "
                          + StringUtils.getFileSizeString(mSubtitle.getContext(), item.terrainTotalSize));
      }
      else
      {
        switch (item.terrainStatus)
        {
        case TERRAIN_ON_DISK ->
        {
          mStatus.setImageResource(R.drawable.ic_check);
          mSubtitle.setText("Downloaded");
        }
        case TERRAIN_PARTLY ->
        {
          mStatus.setImageResource(R.drawable.ic_download);
          mSubtitle.setText("Partly downloaded");
        }
        case TERRAIN_FAILED ->
        {
          mStatus.setImageResource(R.drawable.ic_download);
          mSubtitle.setText("Failed - tap to retry");
        }
        default ->
        {
          mStatus.setImageResource(R.drawable.ic_download);
          mSubtitle.setText("Tap to download");
        }
        }
      }
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
    case CountryItem.STATUS_DOWNLOADABLE: items.add(getDownloadMenuItem()); break;

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
    if (!mSelectedItem.id.startsWith("World"))
    {
      items.add(new MenuBottomSheetItem(R.string.delete, R.drawable.ic_delete,
                                        () -> onDeleteActionSelected(mSelectedItem, DownloaderAdapter.this)));
    }
  }

  private MenuBottomSheetItem getCancelMenuItem()
  {
    return new MenuBottomSheetItem(R.string.cancel, R.drawable.ic_cancel, () -> onCancelActionSelected(mSelectedItem));
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
      case CountryItem.STATUS_DONE, CountryItem.STATUS_PROGRESS, CountryItem.STATUS_APPLYING,
          CountryItem.STATUS_ENQUEUED ->
        processLongClick();
      case CountryItem.STATUS_DOWNLOADABLE, CountryItem.STATUS_PARTLY ->
      {
        if (clickOnStatus)
          onDownloadActionSelected(mItem, DownloaderAdapter.this);
        else
          processLongClick();
      }
      case CountryItem.STATUS_FAILED ->
      {
        MapManagerHelper.warn3gAndRetry(mActivity, mItem.id, null);
      }
      case CountryItem.STATUS_UPDATABLE ->
        MapManagerHelper.warnOn3gUpdate(mActivity, mItem.id, () -> MapManagerHelper.startUpdate(mActivity, mItem.id));
      default -> throw new IllegalArgumentException("Inappropriate item status: " + mItem.status);
      }
    }

    private void processLongClick()
    {
      showBottomSheet(mItem);
    }

    ItemViewHolder(View frame)
    {
      mStatusIcon = new DownloaderStatusIcon(frame.findViewById(R.id.downloader_status_frame)) {
        @Override
        protected int selectIcon(CountryItem country)
        {
          if (country.status == CountryItem.STATUS_DOWNLOADABLE || country.status == CountryItem.STATUS_PARTLY)
          {
            return (country.isExpandable() ? (mMyMapsMode ? R.attr.status_folder_done : R.attr.status_folder)
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
      }.setOnIconClickListener(v -> processClick(true)).setOnCancelClickListener(v -> {
        MapManager.nativeCancel(mItem.id);
        refreshData();
      });

      mName = frame.findViewById(R.id.name);
      mSubtitle = frame.findViewById(R.id.subtitle);
      mFoundName = frame.findViewById(R.id.found_name);
      mSize = frame.findViewById(R.id.size);

      frame.setOnClickListener(v -> {
        if (mItem.isExpandable())
          goDeeper(mItem, true);
        else
          processClick(false);
      });

      frame.setOnLongClickListener(v -> {
        processLongClick();
        return true;
      });
    }

    @Override
    void bind(CountryItem item)
    {
      super.bind(item);

      String found = null;
      if (mSearchResultsMode)
      {
        mName.setText(mItem.name);

        String searchResultName = mItem.searchResultName;
        if (!TextUtils.isEmpty(searchResultName))
        {
          found = StringUtils.toLowerCase(searchResultName);
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
        mName.setText(mItem.name);
        if (!mItem.isExpandable())
          UiUtils.setTextAndHideIfEmpty(mSubtitle, mItem.description);
      }

      if (mItem.isExpandable())
      {
        UiUtils.setTextAndHideIfEmpty(
            mSubtitle,
            String.format("%s: %s", mActivity.getString(R.string.downloader_status_maps),
                          mActivity.getString(R.string.downloader_of, mItem.childCount, mItem.totalChildCount)));
      }

      UiUtils.showIf(mSearchResultsMode && !TextUtils.isEmpty(found), mFoundName);

      long size;
      if (mItem.status == CountryItem.STATUS_ENQUEUED || mItem.status == CountryItem.STATUS_PROGRESS
          || mItem.status == CountryItem.STATUS_APPLYING)
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

  static class HeaderViewHolder extends BaseInnerViewHolder<String>
  {
    @NonNull
    private final TextView mTitle;

    HeaderViewHolder(@NonNull View frame)
    {
      mTitle = frame.findViewById(R.id.title);
    }

    void bind(String text)
    {
      mTitle.setText(text);
    }
  }

  private void collectHeaders()
  {
    mItemsAndHeader.clear();

    int headerId = 0;
    int prev = -1;
    for (CountryItem ci : mItems)
    {
      // Disable headers when using the search
      if (!mSearchResultsMode)
      {
        switch (ci.category)
        {
        case CountryItem.CATEGORY_NEAR_ME ->
        {
          if (ci.category != prev)
          {
            headerId = CountryItem.CATEGORY_NEAR_ME;
            mItemsAndHeader.add(new GenericItem(mActivity.getString(R.string.downloader_near_me_subtitle)));
            prev = ci.category;
          }
        }
        case CountryItem.CATEGORY_DOWNLOADED ->
        {
          if (ci.category != prev)
          {
            headerId = CountryItem.CATEGORY_DOWNLOADED;
            mItemsAndHeader.add(new GenericItem(mActivity.getString(R.string.downloader_downloaded_subtitle)));
            prev = ci.category;
          }
        }
        default ->
        {
          int prevHeader = headerId;
          headerId = CountryItem.CATEGORY_AVAILABLE + ci.name.charAt(0);
          if (headerId != prevHeader)
            mItemsAndHeader.add(new GenericItem(StringUtils.toUpperCase(ci.name.substring(0, 1))));
          prev = ci.category;
        }
        }
        ci.headerId = headerId;
      }
      mItemsAndHeader.add(new GenericItem(ci));
      // The terrain sub-row: the countries.json leafs only (groups aggregate too coarsely).
      if (!ci.isExpandable() && ci.terrainStatus != TerrainViewHolder.TERRAIN_NOT_AVAILABLE)
        mItemsAndHeader.add(new GenericItem(ci, true /* isTerrain */));
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
      Location loc = MwmApplication.from(mActivity).getLocationHelper().getSavedLocation();
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
    mSearchQuery = StringUtils.toLowerCase(query);

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
    for (CountryItem ci : mItems)
    {
      List<CountryItem> lst = mCountryIndex.get(ci.id);
      if (lst != null)
        lst.add(ci);
      else
      {
        lst = new ArrayList<>();
        lst.add(ci);
        mCountryIndex.put(ci.id, lst);
      }
    }

    if (mItems.isEmpty())
      mFragment.setupPlaceholder();

    mFragment.showPlaceholder(mItems.isEmpty());

    notifyDataSetChanged();
  }

  DownloaderAdapter(DownloaderFragment fragment)
  {
    mActivity = fragment.requireActivity();
    mFragment = fragment;
    mRecycler = mFragment.getRecyclerView();
  }

  @NonNull
  private View inflate(ViewGroup parent, @LayoutRes int layoutId)
  {
    return LayoutInflater.from(mActivity).inflate(layoutId, parent, false);
  }

  @Override
  public int getItemViewType(int position)
  {
    final GenericItem item = mItemsAndHeader.get(position);
    if (item.mItem != null)
      return item.mIsTerrain ? TYPE_TERRAIN : TYPE_COUNTRY;
    return TYPE_HEADER;
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
  public int getItemCount()
  {
    return mItemsAndHeader.size();
  }

  private void goDeeper(CountryItem child, boolean refresh)
  {
    LinearLayoutManager lm = (LinearLayoutManager) mRecycler.getLayoutManager();

    // Save scroll positions (top item + item`s offset) for current hierarchy level
    int position = lm.findFirstVisibleItemPosition();
    int offset;

    if (position > -1)
    {
      final View v = lm.findViewByPosition(position);
      offset = v != null ? v.getTop() : 0;
    }
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

    mBackPressedCallback.setEnabled(true);
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

  @NonNull
  OnBackPressedCallback getBackPressedCallback()
  {
    return mBackPressedCallback;
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

  @NonNull
  String getCurrentRootId()
  {
    return (canGoUpwards() ? getCurrentRootItem().id : CountryItem.getRootId());
  }

  @Nullable
  String getCurrentRootName()
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
