package com.mapswithme.maps.bookmarks;

import android.app.Activity;
import android.content.Intent;
import android.location.Location;
import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.CallSuper;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import com.cocosw.bottomsheet.BottomSheet;
import com.google.android.material.floatingactionbutton.FloatingActionButton;
import com.mapswithme.maps.MwmActivity;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmRecyclerFragment;
import com.mapswithme.maps.bookmarks.data.AbstractCategoriesSnapshot;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;
import com.mapswithme.maps.bookmarks.data.BookmarkInfo;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.bookmarks.data.BookmarkSharingResult;
import com.mapswithme.maps.bookmarks.data.CategoryDataSource;
import com.mapswithme.maps.bookmarks.data.SortedBlock;
import com.mapswithme.maps.bookmarks.data.Track;
import com.mapswithme.maps.intent.Factory;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.search.NativeBookmarkSearchListener;
import com.mapswithme.maps.search.SearchEngine;
import com.mapswithme.maps.ugc.routes.BaseUgcRouteActivity;
import com.mapswithme.maps.ugc.routes.UgcRouteEditSettingsActivity;
import com.mapswithme.maps.ugc.routes.UgcRouteSharingOptionsActivity;
import com.mapswithme.maps.widget.SearchToolbarController;
import com.mapswithme.maps.widget.placepage.EditBookmarkFragment;
import com.mapswithme.maps.widget.placepage.Sponsored;
import com.mapswithme.maps.widget.recycler.ItemDecoratorFactory;
import com.mapswithme.util.BottomSheetHelper;
import com.mapswithme.util.CrashlyticsUtils;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.sharing.ShareOption;
import com.mapswithme.util.sharing.SharingHelper;
import com.mapswithme.util.statistics.Statistics;

public class BookmarksListFragment extends BaseMwmRecyclerFragment<BookmarkListAdapter>
    implements BookmarkManager.BookmarksSharingListener,
               BookmarkManager.BookmarksSortingListener,
               NativeBookmarkSearchListener,
               ChooseBookmarksSortingTypeFragment.ChooseSortingTypeListener
{
  public static final String TAG = BookmarksListFragment.class.getSimpleName();
  public static final String EXTRA_CATEGORY = "bookmark_category";

  @SuppressWarnings("NullableProblems")
  @NonNull
  private SearchToolbarController mToolbarController;

  private long mLastQueryTimestamp = 0;
  private long mLastSortTimestamp = 0;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private CategoryDataSource mCategoryDataSource;
  private int mSelectedPosition;

  private boolean mSearchMode = false;
  private boolean mNeedUpdateSorting = true;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private ViewGroup mSearchContainer;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private FloatingActionButton mFabViewOnMap;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private BookmarkManager.BookmarksCatalogListener mCatalogListener;

  @NonNull
  private final RecyclerView.OnScrollListener mRecyclerListener = new RecyclerView.OnScrollListener()
  {
    @Override
    public void onScrollStateChanged(RecyclerView recyclerView, int newState)
    {
      if (newState == RecyclerView.SCROLL_STATE_DRAGGING)
        mToolbarController.deactivate();
    }
  };

  @CallSuper
  @Override
  public void onCreate(@Nullable Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    CrashlyticsUtils.log(Log.INFO, TAG, "onCreate");
    BookmarkCategory category = getCategoryOrThrow();
    mCategoryDataSource = new CategoryDataSource(category);
    mCatalogListener = new CatalogListenerDecorator(this);
  }

  @NonNull
  private BookmarkCategory getCategoryOrThrow()
  {
    Bundle args = getArguments();
    BookmarkCategory category;
    if (args == null || ((category = args.getParcelable(EXTRA_CATEGORY))) == null)
      throw new IllegalArgumentException("Category not exist in bundle");

    return category;
  }

  @NonNull
  @Override
  protected BookmarkListAdapter createAdapter()
  {
    return new BookmarkListAdapter(mCategoryDataSource);
  }

  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    return inflater.inflate(R.layout.fragment_bookmark_list, container, false);
  }

  @CallSuper
  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    CrashlyticsUtils.log(Log.INFO, TAG, "onViewCreated");
    configureAdapter();
    configureFab(view);

    setHasOptionsMenu(true);

    ActionBar bar = ((AppCompatActivity) requireActivity()).getSupportActionBar();
    if (bar != null)
      bar.setTitle(mCategoryDataSource.getData().getName());

    ViewGroup toolbar = ((AppCompatActivity) requireActivity()).findViewById(R.id.toolbar);
    mSearchContainer = toolbar.findViewById(R.id.search_container);
    mToolbarController = new BookmarksToolbarController(toolbar, requireActivity(), this);
    mToolbarController.setHint(R.string.search_in_the_list);
    configureRecycler();
  }

  @Override
  public void onStart()
  {
    super.onStart();
    CrashlyticsUtils.log(Log.INFO, TAG, "onStart");
    SearchEngine.INSTANCE.addBookmarkListener(this);
    BookmarkManager.INSTANCE.addSortingListener(this);
    BookmarkManager.INSTANCE.addSharingListener(this);
    BookmarkManager.INSTANCE.addCatalogListener(mCatalogListener);
  }

  @Override
  public void onResume()
  {
    super.onResume();
    CrashlyticsUtils.log(Log.INFO, TAG, "onResume");
    BookmarkListAdapter adapter = getAdapter();
    adapter.notifyDataSetChanged();
    updateSorting();
    updateSearchVisibility();
    updateRecyclerVisibility();
  }

  @Override
  public void onPause()
  {
    super.onPause();
    CrashlyticsUtils.log(Log.INFO, TAG,"onPause");
  }

  @Override
  public void onStop()
  {
    super.onStop();
    CrashlyticsUtils.log(Log.INFO, TAG, "onStop");
    SearchEngine.INSTANCE.removeBookmarkListener(this);
    BookmarkManager.INSTANCE.removeSortingListener(this);
    BookmarkManager.INSTANCE.removeSharingListener(this);
    BookmarkManager.INSTANCE.removeCatalogListener(mCatalogListener);
  }

  private void configureAdapter()
  {
    BookmarkListAdapter adapter = getAdapter();
    adapter.registerAdapterDataObserver(mCategoryDataSource);
    adapter.setOnClickListener((v, position) ->
    {
      onItemClick(position);
    });
    adapter.setOnLongClickListener((v, position) ->
    {
      onItemMore(position);
    });
    adapter.setMoreListener((v, position) ->
    {
      onItemMore(position);
    });
  }

  private void configureFab(@NonNull View view)
  {
    mFabViewOnMap = view.findViewById(R.id.fabViewOnMap);
    mFabViewOnMap.setOnClickListener(v ->
    {
      final Intent i = makeMwmActivityIntent();
      i.putExtra(MwmActivity.EXTRA_TASK,
          new Factory.ShowBookmarkCategoryTask(mCategoryDataSource.getData().getId()));
      i.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
      startActivity(i);
    });
  }

  private void configureRecycler()
  {
    RecyclerView.ItemDecoration decor = ItemDecoratorFactory
        .createDefaultDecorator(requireContext(), LinearLayoutManager.VERTICAL);
    getRecyclerView().addItemDecoration(decor);
    getRecyclerView().addOnScrollListener(mRecyclerListener);
  }

  private void updateRecyclerVisibility()
  {
    if (isEmptySearchResults())
    {
      requirePlaceholder().setContent(R.drawable.img_search_nothing_found_light,
                                      R.string.search_not_found,
                                      R.string.search_not_found_query);
    }
    else if (isEmpty())
    {
      requirePlaceholder().setContent(R.drawable.img_empty_bookmarks,
                                      R.string.bookmarks_empty_list_title,
                                      R.string.bookmarks_empty_list_message);
    }

    boolean isEmptyRecycler = isEmpty() || isEmptySearchResults();

    showPlaceholder(isEmptyRecycler);
    UiUtils.showIf(!isEmptyRecycler, getRecyclerView(), mFabViewOnMap);
    requireActivity().invalidateOptionsMenu();
  }

  private void updateSearchVisibility()
  {
    if (!isSearchAllowed() || isEmpty())
    {
      UiUtils.hide(mSearchContainer);
    }
    else
    {
      UiUtils.showIf(mSearchMode, mSearchContainer);
      if (mSearchMode)
        mToolbarController.activate();
      else
        mToolbarController.deactivate();
    }
    requireActivity().invalidateOptionsMenu();
  }

  public void runSearch(@NonNull String query)
  {
    SearchEngine.INSTANCE.cancel();

    mLastQueryTimestamp = System.nanoTime();
    if (SearchEngine.INSTANCE.searchInBookmarks(query,
                                                mCategoryDataSource.getData().getId(),
                                                mLastQueryTimestamp))
    {
      mToolbarController.showProgress(true);
    }
  }

  @Override
  public void onBookmarkSearchResultsUpdate(@Nullable long[] bookmarkIds, long timestamp)
  {
    if (!isAdded() || !mToolbarController.hasQuery() || mLastQueryTimestamp != timestamp)
      return;
    updateSearchResults(bookmarkIds);
  }

  @Override
  public void onBookmarkSearchResultsEnd(@Nullable long[] bookmarkIds, long timestamp)
  {
    if (!isAdded() || !mToolbarController.hasQuery() || mLastQueryTimestamp != timestamp)
      return;
    trackBookmarksSearch();
    mLastQueryTimestamp = 0;
    mToolbarController.showProgress(false);
    updateSearchResults(bookmarkIds);
  }

  private void updateSearchResults(@Nullable long[] bookmarkIds)
  {
    BookmarkListAdapter adapter = getAdapter();
    adapter.setSearchResults(bookmarkIds);
    adapter.notifyDataSetChanged();
    updateRecyclerVisibility();
  }

  public void cancelSearch()
  {
    mLastQueryTimestamp = 0;
    SearchEngine.INSTANCE.cancel();
    mToolbarController.showProgress(false);
    updateSearchResults(null);
    updateSorting();
  }

  public void activateSearch()
  {
    mSearchMode = true;
    BookmarkManager.INSTANCE.setNotificationsEnabled(true);
    BookmarkManager.INSTANCE.prepareForSearch(mCategoryDataSource.getData().getId());
    updateSearchVisibility();
  }

  public void deactivateSearch()
  {
    mSearchMode = false;
    BookmarkManager.INSTANCE.setNotificationsEnabled(false);
    updateSearchVisibility();
  }

  @Override
  public void onBookmarksSortingCompleted(@NonNull SortedBlock[] sortedBlocks, long timestamp)
  {
    if (mLastSortTimestamp != timestamp)
      return;
    mLastSortTimestamp = 0;

    BookmarkListAdapter adapter = getAdapter();
    adapter.setSortedResults(sortedBlocks);
    adapter.notifyDataSetChanged();

    updateSortingProgressBar();
  }

  @Override
  public void onBookmarksSortingCancelled(long timestamp)
  {
    if (mLastSortTimestamp != timestamp)
      return;
    mLastSortTimestamp = 0;

    BookmarkListAdapter adapter = getAdapter();
    adapter.setSortedResults(null);
    adapter.notifyDataSetChanged();

    updateSortingProgressBar();
  }

  @Override
  public void onSort(@BookmarkManager.SortingType int sortingType)
  {
    mLastSortTimestamp = System.nanoTime();

    final Location loc = LocationHelper.INSTANCE.getSavedLocation();
    final boolean hasMyPosition = loc != null;
    if (!hasMyPosition && sortingType == BookmarkManager.SORT_BY_DISTANCE)
      return;

    final long catId = mCategoryDataSource.getData().getId();
    final double lat = hasMyPosition ? loc.getLatitude() : 0;
    final double lon = hasMyPosition ? loc.getLongitude() : 0;

    BookmarkManager.INSTANCE.setLastSortingType(catId, sortingType);
    BookmarkManager.INSTANCE.getSortedCategory(catId, sortingType, hasMyPosition, lat, lon,
                                               mLastSortTimestamp);

    updateSortingProgressBar();
  }

  @Override
  public void onResetSorting()
  {
    mLastSortTimestamp = 0;
    long catId = mCategoryDataSource.getData().getId();
    BookmarkManager.INSTANCE.resetLastSortingType(catId);

    BookmarkListAdapter adapter = getAdapter();
    adapter.setSortedResults(null);
    adapter.notifyDataSetChanged();
  }

  private void updateSorting()
  {
    if (!mNeedUpdateSorting)
      return;
    mNeedUpdateSorting = false;

    // Do nothing in case of sorting has already started and we are waiting for results.
    if (mLastSortTimestamp != 0)
      return;

    long catId = mCategoryDataSource.getData().getId();
    if (!BookmarkManager.INSTANCE.hasLastSortingType(catId))
      return;

    int currentType = getLastAvailableSortingType();
    if (currentType >= 0)
      onSort(currentType);
  }

  private void forceUpdateSorting()
  {
    mLastSortTimestamp = 0;
    mNeedUpdateSorting = true;
    updateSorting();
  }

  private void resetSearchAndSort()
  {
    BookmarkListAdapter adapter = getAdapter();
    adapter.setSortedResults(null);
    adapter.setSearchResults(null);
    adapter.notifyDataSetChanged();

    if (mSearchMode)
    {
      cancelSearch();
      deactivateSearch();
    }
    forceUpdateSorting();
    updateRecyclerVisibility();
  }

  @NonNull
  @BookmarkManager.SortingType
  private int[] getAvailableSortingTypes()
  {
    final long catId = mCategoryDataSource.getData().getId();
    final Location loc = LocationHelper.INSTANCE.getSavedLocation();
    final boolean hasMyPosition = loc != null;
    return BookmarkManager.INSTANCE.getAvailableSortingTypes(catId, hasMyPosition);
  }

  private int getLastSortingType()
  {
    final long catId = mCategoryDataSource.getData().getId();
    if (BookmarkManager.INSTANCE.hasLastSortingType(catId))
      return BookmarkManager.INSTANCE.getLastSortingType(catId);
    return -1;
  }

  private int getLastAvailableSortingType()
  {
    int currentType = getLastSortingType();
    @BookmarkManager.SortingType int[] types = getAvailableSortingTypes();
    for (@BookmarkManager.SortingType int type : types)
    {
      if (type == currentType)
        return currentType;
    }
    return -1;
  }

  private boolean isEmpty()
  {
    return !getAdapter().isSearchResults() && getAdapter().getItemCount() == 0;
  }

  private boolean isEmptySearchResults()
  {
    return getAdapter().isSearchResults() && getAdapter().getItemCount() == 0;
  }

  private boolean isDownloadedCategory()
  {
    BookmarkCategory category = mCategoryDataSource.getData();
    return category.getType() == BookmarkCategory.Type.DOWNLOADED;
  }

  private boolean isSearchAllowed()
  {
    BookmarkCategory category = mCategoryDataSource.getData();
    return BookmarkManager.INSTANCE.isSearchAllowed(category.getId());
  }

  private boolean isLastOwnedCategory()
  {
    AbstractCategoriesSnapshot snapshot = BookmarkManager.INSTANCE.getOwnedCategoriesSnapshot();
    return snapshot.getItems().size() == 1;
  }

  private void updateSortingProgressBar()
  {
    requireActivity().invalidateOptionsMenu();
  }

  public void onItemClick(int position)
  {
    final Intent intent = makeMwmActivityIntent();

    BookmarkListAdapter adapter = getAdapter();

    switch (adapter.getItemViewType(position))
    {
      case BookmarkListAdapter.TYPE_SECTION:
      case BookmarkListAdapter.TYPE_DESC:
        return;

      case BookmarkListAdapter.TYPE_BOOKMARK:
        onBookmarkClicked(position, intent, adapter);
        break;

      case BookmarkListAdapter.TYPE_TRACK:
        onTrackClicked(position, intent, adapter);
        break;
    }

    intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
    startActivity(intent);
  }

  @NonNull
  private Intent makeMwmActivityIntent()
  {
    return new Intent(requireActivity(), MwmActivity.class);
  }

  private void onTrackClicked(int position, @NonNull Intent i, @NonNull BookmarkListAdapter adapter)
  {
    final Track track = (Track) adapter.getItem(position);
    i.putExtra(MwmActivity.EXTRA_TASK,
               new Factory.ShowTrackTask(track.getCategoryId(), track.getTrackId()));
    if (BookmarkManager.INSTANCE.isGuide(mCategoryDataSource.getData()))
      Statistics.INSTANCE.trackGuideTrackSelect(mCategoryDataSource.getData().getServerId());
  }

  private void onBookmarkClicked(int position, @NonNull Intent i,
                                 @NonNull BookmarkListAdapter adapter)
  {
    if (getAdapter().isSearchResults())
      trackBookmarksSearchResultSelected();

    final BookmarkInfo bookmark = (BookmarkInfo) adapter.getItem(position);
    i.putExtra(MwmActivity.EXTRA_TASK,
               new Factory.ShowBookmarkTask(bookmark.getCategoryId(), bookmark.getBookmarkId()));

    if (BookmarkManager.INSTANCE.isGuide(mCategoryDataSource.getData()))
      Statistics.INSTANCE.trackGuideBookmarkSelect(mCategoryDataSource.getData().getServerId());
  }

  public void onItemMore(int position)
  {
    BookmarkListAdapter adapter = getAdapter();

    mSelectedPosition = position;
    int type = adapter.getItemViewType(mSelectedPosition);

    switch (type)
    {
      case BookmarkListAdapter.TYPE_SECTION:
      case BookmarkListAdapter.TYPE_DESC:
        // Do nothing here?
        break;

      case BookmarkListAdapter.TYPE_BOOKMARK:
        final BookmarkInfo bookmark = (BookmarkInfo) adapter.getItem(mSelectedPosition);
        int menuResId = isDownloadedCategory() ? R.menu.menu_bookmarks_catalog
            : R.menu.menu_bookmarks;
        BottomSheet bs = BottomSheetHelper.create(requireActivity(), bookmark.getName())
            .sheet(menuResId)
            .listener(this::onBookmarkMenuItemClicked)
            .build();
        BottomSheetHelper.tint(bs);
        bs.show();
        break;

      case BookmarkListAdapter.TYPE_TRACK:
        final Track track = (Track) adapter.getItem(mSelectedPosition);
        BottomSheet bottomSheet = BottomSheetHelper
            .create(requireActivity(), track.getName())
            .sheet(Menu.NONE, R.drawable.ic_delete, R.string.delete)
            .listener(menuItem -> onTrackMenuItemClicked(track.getTrackId()))
            .build();

        BottomSheetHelper.tint(bottomSheet);
        bottomSheet.show();
        break;
    }
  }

  private boolean onTrackMenuItemClicked(long trackId)
  {
    BookmarkManager.INSTANCE.deleteTrack(trackId);
    getAdapter().notifyDataSetChanged();
    return false;
  }

  public boolean onBookmarkMenuItemClicked(@NonNull MenuItem menuItem)
  {
    BookmarkListAdapter adapter = getAdapter();
    BookmarkInfo item = (BookmarkInfo) adapter.getItem(mSelectedPosition);
    switch (menuItem.getItemId())
    {
      case R.id.share:
        ShareOption.AnyShareOption.ANY.shareBookmarkObject(requireActivity(), item,
                                                           Sponsored.nativeGetCurrent());
        break;

      case R.id.edit:
        EditBookmarkFragment.editBookmark(
            item.getCategoryId(), item.getBookmarkId(), requireActivity(), getChildFragmentManager(),
            (bookmarkId, movedFromCategory) ->
            {
              if (movedFromCategory)
                resetSearchAndSort();
              else
                adapter.notifyDataSetChanged();
            });
        break;

      case R.id.delete:
        adapter.onDelete(mSelectedPosition);
        BookmarkManager.INSTANCE.deleteBookmark(item.getBookmarkId());
        adapter.notifyDataSetChanged();
        if (mSearchMode)
          mNeedUpdateSorting = true;
        updateSearchVisibility();
        updateRecyclerVisibility();
        break;
    }
    return false;
  }

  public boolean onListMoreMenuItemClick(@NonNull MenuItem menuItem)
  {
    switch (menuItem.getItemId())
    {
      case R.id.sort:
        ChooseBookmarksSortingTypeFragment.chooseSortingType(getAvailableSortingTypes(),
            getLastSortingType(), requireActivity(), getChildFragmentManager());
        return false;

      case R.id.sharing_options:
        openSharingOptionsScreen();
        trackBookmarkListSharingOptions();
        return false;

      case R.id.share_category:
        long catId = mCategoryDataSource.getData().getId();
        SharingHelper.INSTANCE.prepareBookmarkCategoryForSharing(requireActivity(), catId);
        return false;

      case R.id.settings:
        Intent intent = new Intent(requireContext(), UgcRouteEditSettingsActivity.class).putExtra(
            BaseUgcRouteActivity.EXTRA_BOOKMARK_CATEGORY,
            mCategoryDataSource.getData());
        startActivityForResult(intent, UgcRouteEditSettingsActivity.REQUEST_CODE);
        return false;

      case R.id.delete_category:
        requireActivity().setResult(Activity.RESULT_OK);
        requireActivity().finish();
        return false;
    }
    return false;
  }

  @Override
  public void onCreateOptionsMenu(Menu menu, MenuInflater inflater)
  {
    inflater.inflate(R.menu.option_menu_bookmarks, menu);

    MenuItem itemSearch = menu.findItem(R.id.bookmarks_search);
    itemSearch.setVisible(isSearchAllowed() && !isEmpty());

    MenuItem itemMore = menu.findItem(R.id.bookmarks_more);
    itemMore.setVisible(!isEmpty());
  }

  @Override
  public void onPrepareOptionsMenu(Menu menu)
  {
    super.onPrepareOptionsMenu(menu);

    boolean visible = !mSearchMode && !isEmpty();
    MenuItem itemSearch = menu.findItem(R.id.bookmarks_search);
    itemSearch.setVisible(isSearchAllowed() && visible);

    MenuItem itemMore = menu.findItem(R.id.bookmarks_more);
    itemMore.setVisible(visible);

    if (mLastSortTimestamp != 0)
      itemMore.setActionView(R.layout.toolbar_menu_progressbar);
  }

  @Override
  public boolean onOptionsItemSelected(MenuItem item)
  {
    if (item.getItemId() == R.id.bookmarks_search)
    {
      activateSearch();
      return true;
    }

    if (item.getItemId() == R.id.bookmarks_more)
    {
      BottomSheet bs = BottomSheetHelper.create(requireActivity(),
          mCategoryDataSource.getData().getName())
          .sheet(R.menu.menu_bookmarks_list)
          .listener(this::onListMoreMenuItemClick)
          .build();

      if (isDownloadedCategory())
      {
        bs.getMenu().findItem(R.id.sharing_options).setVisible(false);
        bs.getMenu().findItem(R.id.share_category).setVisible(false);
        bs.getMenu().findItem(R.id.settings).setVisible(false);
      }

      @BookmarkManager.SortingType int[] types = getAvailableSortingTypes();
      bs.getMenu().findItem(R.id.sort).setVisible(types.length > 0);
      bs.getMenu().findItem(R.id.delete_category).setVisible(!isLastOwnedCategory());

      BottomSheetHelper.tint(bs);
      bs.show();
      return true;
    }

    return super.onOptionsItemSelected(item);
  }

  @Override
  public void onPreparedFileForSharing(@NonNull BookmarkSharingResult result)
  {
    SharingHelper.INSTANCE.onPreparedFileForSharing(requireActivity(), result);
  }

  private void openSharingOptionsScreen()
  {
    UgcRouteSharingOptionsActivity.startForResult(requireActivity(), mCategoryDataSource.getData());
  }

  private static void trackBookmarkListSharingOptions()
  {
    Statistics.INSTANCE.trackBookmarkListSharingOptions();
  }

  private static void trackBookmarksSearch()
  {
    Statistics.INSTANCE.trackBookmarksListSearch();
  }

  private static void trackBookmarksSearchResultSelected()
  {
    Statistics.INSTANCE.trackBookmarksListSearchResultSelected();
  }

  @SuppressWarnings("ConstantConditions")
  @Override
  public void onActivityResult(int requestCode, int resultCode, Intent data)
  {
    super.onActivityResult(requestCode, resultCode, data);
    getAdapter().notifyDataSetChanged();
    ActionBar actionBar = ((AppCompatActivity) requireActivity()).getSupportActionBar();
    actionBar.setTitle(mCategoryDataSource.getData().getName());
  }
}
