package com.mapswithme.maps.bookmarks;

import android.content.Intent;
import android.os.Bundle;
import android.support.annotation.CallSuper;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
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
import com.mapswithme.maps.search.NativeBookmarkSearchListener;
import com.mapswithme.maps.search.SearchEngine;
import com.mapswithme.maps.ugc.routes.BaseUgcRouteActivity;
import com.mapswithme.maps.ugc.routes.UgcRouteEditSettingsActivity;
import com.mapswithme.maps.ugc.routes.UgcRouteSharingOptionsActivity;
import com.mapswithme.maps.widget.SearchToolbarController;
import com.mapswithme.maps.widget.placepage.EditBookmarkFragment;
import com.mapswithme.maps.widget.placepage.Sponsored;
import com.mapswithme.maps.widget.recycler.ItemDecoratorFactory;
import com.mapswithme.maps.widget.recycler.RecyclerClickListener;
import com.mapswithme.maps.widget.recycler.RecyclerLongClickListener;
import com.mapswithme.util.BottomSheetHelper;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.sharing.ShareOption;
import com.mapswithme.util.sharing.SharingHelper;
import com.mapswithme.util.statistics.Statistics;

import java.util.Arrays;
import java.util.ArrayList;
import java.util.List;

public class BookmarksListFragment extends BaseMwmRecyclerFragment<BookmarkListAdapter>
    implements RecyclerLongClickListener,
               RecyclerClickListener,
               MenuItem.OnMenuItemClickListener,
               BookmarkManager.BookmarksSharingListener,
               BookmarkManager.BookmarksSortingListener,
               NativeBookmarkSearchListener
{
  public static final String TAG = BookmarksListFragment.class.getSimpleName();
  public static final String EXTRA_CATEGORY = "bookmark_category";

  private BookmarksToolbarController mToolbarController;
  private long mLastQueryTimestamp;
  private long mLastSortTimestamp;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private CategoryDataSource mCategoryDataSource;
  private int mSelectedPosition;

  private List<Long> mSearchResults;
  private List<SortedBlock> mSortResults;

  private boolean mSearchMode = false;
  private boolean mSortMode = false;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private BookmarkManager.BookmarksCatalogListener mCatalogListener;

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
    View root = inflater.inflate(R.layout.fragment_bookmark_list, container, false);
    return root;
  }

  @CallSuper
  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    Crashlytics.log("onViewCreated");
    configureAdapter();
    setHasOptionsMenu(true);
    boolean isEmpty = getAdapter().getItemCount() == 0;
    UiUtils.showIf(!isEmpty, getRecyclerView());
    showPlaceholder(isEmpty);
    ActionBar bar = ((AppCompatActivity) getActivity()).getSupportActionBar();
    if (bar != null)
      bar.setTitle(mCategoryDataSource.getData().getName());

    ViewGroup toolbar = ((AppCompatActivity) getActivity()).findViewById(R.id.toolbar);
    mToolbarController = new BookmarksToolbarController(toolbar, getActivity(), this);

    addRecyclerDecor();
  }

  private void addRecyclerDecor()
  {
    RecyclerView.ItemDecoration decor = ItemDecoratorFactory
        .createDefaultDecorator(getContext(), LinearLayoutManager.VERTICAL);
    getRecyclerView().addItemDecoration(decor);
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
    adapter.setOnClickListener(this);
    adapter.setOnLongClickListener(isDownloadedCategory() ? null : this);
  }

  @Override
  public void onItemClick(View v, int position)
  {
    final Intent i = new Intent(getActivity(), MwmActivity.class);

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

  @Override
  public void onLongItemClick(View v, int position)
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
        BottomSheet bs = BottomSheetHelper.create(getActivity(), bookmark.getTitle())
                                          .sheet(menuResId)
                                          .listener(this)
                                          .build();
        BottomSheetHelper.tint(bs);
        bs.show();
        break;

      case BookmarkListAdapter.TYPE_TRACK:
        final Track track = (Track) adapter.getItem(mSelectedPosition);
        BottomSheet bottomSheet = BottomSheetHelper
            .create(getActivity(), track.getName())
            .sheet(Menu.NONE, R.drawable.ic_delete, R.string.delete)
            .listener(menuItem -> onMenuItemClicked(adapter, track))
            .build();

        BottomSheetHelper.tint(bottomSheet);
        bottomSheet.show();
        break;
    }
  }

  private boolean onMenuItemClicked(@NonNull BookmarkListAdapter adapter, @NonNull Track track)
  {
    BookmarkManager.INSTANCE.deleteTrack(track.getTrackId());
    adapter.notifyDataSetChanged();
    return false;
  }

  private void updateControlsStatus()
  {
    ViewGroup toolbar = ((AppCompatActivity) getActivity()).findViewById(R.id.toolbar);
    ViewGroup searchContainer = toolbar.findViewById(R.id.toolbar_search_container);
    UiUtils.showIf(isSearchMode(), searchContainer);
    if (isSearchMode())
    {
      mToolbarController.activate();
    }
    else
    {
      mToolbarController.deactivate();
    }

    getActivity().invalidateOptionsMenu();
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
    if (!isAdded() || !mToolbarController.hasQuery())
      return;
    updateSearchResults(bookmarkIds);
  }

  @Override
  public void onBookmarkSearchResultsEnd(@Nullable long[] bookmarkIds, long timestamp)
  {
    if (!isAdded() || !mToolbarController.hasQuery())
      return;
    mToolbarController.showProgress(false);
    updateSearchResults(bookmarkIds);
  }

  private void updateSearchResults(@Nullable long[] bookmarkIds)
  {
    ArrayList<Long> ids = new ArrayList<Long>(bookmarkIds.length);
    for (long id : bookmarkIds)
      ids.add(id);

    mSearchResults = ids;

    BookmarkListAdapter adapter = getAdapter();
    adapter.setSearchResults(mSearchResults);
    adapter.notifyDataSetChanged();
  }

  public void cancelSearch()
  {
    SearchEngine.INSTANCE.cancel();
    mToolbarController.showProgress(false);
    mSearchResults = null;
    BookmarkListAdapter adapter = getAdapter();
    adapter.setSearchResults(mSearchResults);
    adapter.notifyDataSetChanged();
  }

  public void activateSearch()
  {
    mSearchMode = true;

    updateControlsStatus();
  }

  public void deactivateSearch()
  {
    mSearchMode = false;

    updateControlsStatus();
  }

  public boolean isSearchMode()
  {
    return mSearchMode;
  }

  @Override
  public void onBookmarksSortingCompleted(SortedBlock[] sortedBlocks, long timestamp)
  {
    if (mLastSortTimestamp != timestamp)
      return;

    mSortResults = Arrays.asList(sortedBlocks);

    BookmarkListAdapter adapter = getAdapter();
    adapter.setSortedResults(mSortResults);
    adapter.notifyDataSetChanged();
  }

  @Override
  public void onBookmarksSortingCancelled(long timestamp)
  {
    if (mLastSortTimestamp != timestamp)
      return;

    mSortResults = null;

    BookmarkListAdapter adapter = getAdapter();
    adapter.setSortedResults(mSortResults);
    adapter.notifyDataSetChanged();
  }

  @Override
  public void onPreparedFileForSharing(@NonNull BookmarkSharingResult result)
  {
    SharingHelper.INSTANCE.onPreparedFileForSharing(getActivity(), result);
  }

  @Override
  public boolean onMenuItemClick(MenuItem menuItem)
  {
    switch (menuItem.getItemId())
    {
      case R.id.sort:
        mLastSortTimestamp = System.nanoTime();
        BookmarkManager.INSTANCE.getSortedBookmarks(
            getCategoryOrThrow().getId(), BookmarkManager.SORT_BY_TYPE,
            false, 0, 0, mLastSortTimestamp);
        return false;
      case R.id.sharing_options:
        return false;
      case R.id.share_category:
        return false;
      case R.id.settings:
        Intent intent = new Intent(getContext(), UgcRouteEditSettingsActivity.class).putExtra(
            BaseUgcRouteActivity.EXTRA_BOOKMARK_CATEGORY,
            getCategoryOrThrow());
        startActivityForResult(intent, UgcRouteEditSettingsActivity.REQUEST_CODE);
        return false;
      case R.id.delete_category:
        return false;
    }

    BookmarkListAdapter adapter = getAdapter();

    Bookmark item = (Bookmark) adapter.getItem(mSelectedPosition);

    switch (menuItem.getItemId())
    {
      case R.id.share:
        ShareOption.ANY.shareMapObject(getActivity(), item, Sponsored.nativeGetCurrent());
        break;

      case R.id.edit:
        EditBookmarkFragment.editBookmark(item.getCategoryId(), item.getBookmarkId(), getActivity(),
                                          getChildFragmentManager(),
                                          bookmarkId -> adapter.notifyDataSetChanged());
        break;

      case R.id.delete:
        BookmarkManager.INSTANCE.deleteBookmark(item.getBookmarkId());
        adapter.notifyDataSetChanged();
        break;
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
    itemSearch.setVisible(getCategoryOrThrow().isSearchAllowed());
  }

  @Override
  public void onPrepareOptionsMenu(Menu menu)
  {
    super.onPrepareOptionsMenu(menu);
    //menu.setGroupVisible(0, false);
    final boolean visible = !isSearchMode();
    MenuItem itemSearch = menu.findItem(R.id.bookmarks_search);
    itemSearch.setVisible(visible);

    MenuItem itemMore = menu.findItem(R.id.bookmarks_more);
    itemMore.setVisible(visible);
  }

  @SuppressWarnings("ConstantConditions")
  @Override
  public void onActivityResult(int requestCode, int resultCode, Intent data)
  {
    super.onActivityResult(requestCode, resultCode, data);
    getAdapter().notifyDataSetChanged();
    ActionBar actionBar = ((AppCompatActivity) getActivity()).getSupportActionBar();
    actionBar.setTitle(mCategoryDataSource.getData().getName());
  }

  private boolean isDownloadedCategory()
  {
    BookmarkCategory category = mCategoryDataSource.getData();
    return category.getType() == BookmarkCategory.Type.DOWNLOADED;
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
      //openSharingOptionsScreen();
      //trackBookmarkListSharingOptions();
      BottomSheet bs = BottomSheetHelper.create(getActivity(),
                                                mCategoryDataSource.getData().getName())
                                        .sheet(R.menu.menu_bookmarks_list)
                                        .listener(this)
                                        .build();
      BottomSheetHelper.tint(bs);
      bs.show();
      return true;
    }

    return super.onOptionsItemSelected(item);
  }

  private void trackBookmarkListSharingOptions()
  {
    Statistics.INSTANCE.trackBookmarkListSharingOptions();
  }

  private void openSharingOptionsScreen()
  {
    UgcRouteSharingOptionsActivity.startForResult(getActivity(), mCategoryDataSource.getData());
  }
}
