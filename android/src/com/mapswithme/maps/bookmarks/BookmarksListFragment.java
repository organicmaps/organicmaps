package com.mapswithme.maps.bookmarks;

import android.content.Intent;
import android.location.Location;
import android.os.Bundle;
import android.support.annotation.CallSuper;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.design.widget.FloatingActionButton;
import android.support.v7.app.ActionBar;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;

import com.cocosw.bottomsheet.BottomSheet;
import com.crashlytics.android.Crashlytics;
import com.mapswithme.maps.MwmActivity;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmRecyclerFragment;
import com.mapswithme.maps.bookmarks.data.Bookmark;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;
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
    Crashlytics.log("onCreate");
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
    Crashlytics.log("onViewCreated");
    configureAdapter();
    configureFab(view);

    setHasOptionsMenu(true);

    ActionBar bar = ((AppCompatActivity) requireActivity()).getSupportActionBar();
    if (bar != null)
      bar.setTitle(mCategoryDataSource.getData().getName());

    ViewGroup toolbar = ((AppCompatActivity) requireActivity()).findViewById(R.id.toolbar);
    mSearchContainer = toolbar.findViewById(R.id.frame);
    mToolbarController = new BookmarksToolbarController(toolbar, requireActivity(), this);

    configureRecycler();
  }

  @Override
  public void onStart()
  {
    super.onStart();
    Crashlytics.log("onStart");
    SearchEngine.INSTANCE.addBookmarkListener(this);
    BookmarkManager.INSTANCE.addSortingListener(this);
    BookmarkManager.INSTANCE.addSharingListener(this);
    BookmarkManager.INSTANCE.addCatalogListener(mCatalogListener);
  }

  @Override
  public void onResume()
  {
    super.onResume();
    Crashlytics.log("onResume");
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
    Crashlytics.log("onPause");
  }

  @Override
  public void onStop()
  {
    super.onStop();
    Crashlytics.log("onStop");
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
      final Intent i = new Intent(requireActivity(), MwmActivity.class);
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
    return !isDownloadedCategory() && BookmarkManager.INSTANCE.isSearchAllowed(category.getId());
  }

  public void onItemClick(int position)
  {
    final Intent i = new Intent(requireActivity(), MwmActivity.class);

    BookmarkListAdapter adapter = getAdapter();

    switch (adapter.getItemViewType(position))
    {
      case BookmarkListAdapter.TYPE_SECTION:
      case BookmarkListAdapter.TYPE_DESC:
        return;

      case BookmarkListAdapter.TYPE_BOOKMARK:
        final Bookmark bookmark = (Bookmark) adapter.getItem(position);
        i.putExtra(MwmActivity.EXTRA_TASK,
            new Factory.ShowBookmarkTask(bookmark.getCategoryId(), bookmark.getBookmarkId()));
        break;

      case BookmarkListAdapter.TYPE_TRACK:
        final Track track = (Track) adapter.getItem(position);
        i.putExtra(MwmActivity.EXTRA_TASK,
            new Factory.ShowTrackTask(track.getCategoryId(), track.getTrackId()));
        break;
    }

    i.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
    startActivity(i);
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
        final Bookmark bookmark = (Bookmark) adapter.getItem(mSelectedPosition);
        int menuResId = isDownloadedCategory() ? R.menu.menu_bookmarks_catalog
            : R.menu.menu_bookmarks;
        BottomSheet bs = BottomSheetHelper.create(requireActivity(), bookmark.getTitle())
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
    Bookmark item = (Bookmark) adapter.getItem(mSelectedPosition);
    switch (menuItem.getItemId())
    {
      case R.id.share:
        ShareOption.ANY.shareMapObject(requireActivity(), item, Sponsored.nativeGetCurrent());
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
        return false;
    }
    return false;
  }

  @Override
  public void onCreateOptionsMenu(Menu menu, MenuInflater inflater)
  {
    if (isDownloadedCategory())
      return;

    inflater.inflate(R.menu.option_menu_bookmarks, menu);

    MenuItem itemSearch = menu.findItem(R.id.bookmarks_search);
    itemSearch.setVisible(isSearchAllowed() && !isEmpty());

    MenuItem itemMore = menu.findItem(R.id.bookmarks_more);
    itemMore.setVisible(!isEmpty());
  }

  @Override
  public void onPrepareOptionsMenu(Menu menu)
  {
    if (isDownloadedCategory())
      return;

    super.onPrepareOptionsMenu(menu);

    boolean visible = !mSearchMode && !isEmpty();
    MenuItem itemSearch = menu.findItem(R.id.bookmarks_search);
    itemSearch.setVisible(isSearchAllowed() && visible);

    MenuItem itemMore = menu.findItem(R.id.bookmarks_more);
    itemMore.setVisible(visible);
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

      @BookmarkManager.SortingType int[] types = getAvailableSortingTypes();
      bs.getMenu().findItem(R.id.sort).setVisible(types.length > 0);

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

  private void trackBookmarkListSharingOptions()
  {
    Statistics.INSTANCE.trackBookmarkListSharingOptions();
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
