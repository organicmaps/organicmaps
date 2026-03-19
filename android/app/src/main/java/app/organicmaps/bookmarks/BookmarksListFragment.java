package app.organicmaps.bookmarks;

import android.app.Activity;
import android.content.Intent;
import android.location.Location;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.annotation.CallSuper;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.view.MenuProvider;
import androidx.core.view.ViewCompat;
import androidx.recyclerview.widget.ConcatAdapter;
import androidx.recyclerview.widget.RecyclerView;
import androidx.recyclerview.widget.SimpleItemAnimator;
import app.organicmaps.MwmActivity;
import app.organicmaps.MwmApplication;
import app.organicmaps.R;
import app.organicmaps.base.BaseMwmRecyclerFragment;
import app.organicmaps.sdk.bookmarks.data.BookmarkCategory;
import app.organicmaps.sdk.bookmarks.data.BookmarkInfo;
import app.organicmaps.sdk.bookmarks.data.BookmarkManager;
import app.organicmaps.sdk.bookmarks.data.BookmarkSharingResult;
import app.organicmaps.sdk.bookmarks.data.CategoryDataSource;
import app.organicmaps.sdk.bookmarks.data.FileType;
import app.organicmaps.sdk.bookmarks.data.Icon;
import app.organicmaps.sdk.bookmarks.data.PredefinedColors;
import app.organicmaps.sdk.bookmarks.data.SortedBlock;
import app.organicmaps.sdk.bookmarks.data.Track;
import app.organicmaps.sdk.search.BookmarkSearchListener;
import app.organicmaps.sdk.search.SearchEngine;
import app.organicmaps.util.SharingUtils;
import app.organicmaps.util.UiUtils;
import app.organicmaps.util.Utils;
import app.organicmaps.util.WindowInsetUtils;
import app.organicmaps.util.bottomsheet.MenuBottomSheetFragment;
import app.organicmaps.util.bottomsheet.MenuBottomSheetItem;
import app.organicmaps.widget.SearchToolbarController;
import app.organicmaps.widget.placepage.BookmarkColorDialogFragment;
import app.organicmaps.widget.placepage.EditBookmarkFragment;
import app.organicmaps.widget.recycler.DividerItemDecorationWithPadding;
import com.google.android.material.floatingactionbutton.ExtendedFloatingActionButton;
import java.util.ArrayList;
import java.util.List;
import java.util.Objects;

public class BookmarksListFragment extends BaseMwmRecyclerFragment<ConcatAdapter>
    implements BookmarkManager.BookmarksSharingListener, BookmarkManager.BookmarksSortingListener,
               BookmarkManager.BookmarksLoadingListener, BookmarkSearchListener,
               ChooseBookmarksSortingTypeFragment.ChooseSortingTypeListener,
               MenuBottomSheetFragment.MenuBottomSheetInterface,
               BookmarkColorDialogFragment.OnBookmarkColorChangeListener
{
  public static final String TAG = BookmarksListFragment.class.getSimpleName();
  public static final String EXTRA_CATEGORY = "bookmark_category";
  private static final int INDEX_BOOKMARKS_COLLECTION_ADAPTER = 0;
  private static final int INDEX_BOOKMARKS_LIST_ADAPTER = 1;
  private static final String BOOKMARKS_MENU_ID = "BOOKMARKS_MENU_BOTTOM_SHEET";
  private static final String TRACK_MENU_ID = "TRACK_MENU_BOTTOM_SHEET";
  private static final String OPTIONS_MENU_ID = "OPTIONS_MENU_BOTTOM_SHEET";
  private static final String EXTRA_SELECTED_ITEM_ID = "selected_item_id";
  private static final String EXTRA_SELECTED_ITEM_TYPE = "selected_item_type";

  private ActivityResultLauncher<SharingUtils.SharingIntent> shareLauncher;
  private final ActivityResultLauncher<Intent> startBookmarkListForResult = registerForActivityResult(
      new ActivityResultContracts.StartActivityForResult(), activityResult -> { handleActivityResult(); });

  private final ActivityResultLauncher<Intent> startBookmarkSettingsForResult = registerForActivityResult(
      new ActivityResultContracts.StartActivityForResult(), activityResult -> { handleActivityResult(); });

  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private SearchToolbarController mToolbarController;
  private long mLastQueryTimestamp = 0;
  private long mLastSortTimestamp = 0;
  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private CategoryDataSource mCategoryDataSource;
  private long mSelectedItemId = -1;
  private int mSelectedItemType = -1;
  private boolean mSearchMode = false;
  private boolean mNeedUpdateSorting = true;
  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private ViewGroup mSearchContainer;
  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private ExtendedFloatingActionButton mFabViewOnMap;
  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private final RecyclerView.OnScrollListener mRecyclerListener = new RecyclerView.OnScrollListener() {
    @Override
    public void onScrollStateChanged(@NonNull RecyclerView recyclerView, int newState)
    {
      if (newState == RecyclerView.SCROLL_STATE_DRAGGING)
        mToolbarController.deactivate();
    }
  };
  @Nullable
  private Bundle mSavedInstanceState;

  @NonNull
  private final MenuProvider mMenuProvider = new MenuProvider() {
    @Override
    public void onPrepareMenu(@NonNull Menu menu)
    {
      final boolean visible = !mSearchMode && !isEmpty();
      final MenuItem itemSearch = menu.findItem(R.id.bookmarks_search);
      itemSearch.setVisible(visible);

      final MenuItem itemMore = menu.findItem(R.id.bookmarks_more);
      if (mLastSortTimestamp != 0)
        itemMore.setActionView(R.layout.toolbar_menu_progressbar);
    }

    @Override
    public void onCreateMenu(@NonNull Menu menu, @NonNull MenuInflater menuInflater)
    {
      menuInflater.inflate(R.menu.option_menu_bookmarks, menu);

      menu.findItem(R.id.bookmarks_search).setVisible(!isEmpty());
    }

    @Override
    public boolean onMenuItemSelected(@NonNull MenuItem menuItem)
    {
      if (menuItem.getItemId() == R.id.bookmarks_search)
      {
        activateSearch();
        return true;
      }

      if (menuItem.getItemId() == R.id.bookmarks_more)
      {
        MenuBottomSheetFragment.newInstance(OPTIONS_MENU_ID, mCategoryDataSource.getData().getName())
            .show(getChildFragmentManager(), OPTIONS_MENU_ID);
        return true;
      }
      return false;
    }
  };

  @CallSuper
  @Override
  public void onCreate(@Nullable Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    BookmarkCategory category = getCategoryOrThrow();
    mCategoryDataSource = new CategoryDataSource(category);

    if (savedInstanceState != null)
    {
      mSelectedItemId = savedInstanceState.getLong(EXTRA_SELECTED_ITEM_ID, -1);
      mSelectedItemType = savedInstanceState.getInt(EXTRA_SELECTED_ITEM_TYPE, -1);
    }

    shareLauncher = SharingUtils.RegisterLauncher(this);
  }

  @Override
  public void onSaveInstanceState(@NonNull Bundle outState)
  {
    super.onSaveInstanceState(outState);
    outState.putLong(EXTRA_SELECTED_ITEM_ID, mSelectedItemId);
    outState.putInt(EXTRA_SELECTED_ITEM_TYPE, mSelectedItemType);
  }

  @NonNull
  private BookmarkCategory getCategoryOrThrow()
  {
    final Bundle args = requireArguments();
    return Objects.requireNonNull(Utils.getParcelable(args, EXTRA_CATEGORY, BookmarkCategory.class));
  }

  @NonNull
  @Override
  protected ConcatAdapter createAdapter()
  {
    BookmarkCategory category = mCategoryDataSource.getData();
    return new ConcatAdapter(initAndGetCollectionAdapter(category.getId()),
                             new BookmarkListAdapter(mCategoryDataSource));
  }

  @NonNull
  private RecyclerView.Adapter<RecyclerView.ViewHolder> initAndGetCollectionAdapter(long categoryId)
  {
    List<BookmarkCategory> mCategoryItems = BookmarkManager.INSTANCE.getChildrenCategories(categoryId);

    BookmarkCollectionAdapter adapter = new BookmarkCollectionAdapter(getCategoryOrThrow(), mCategoryItems);
    adapter.setOnClickListener(
        (v, item) -> { BookmarkListActivity.startForResult(this, startBookmarkListForResult, item); });

    return adapter;
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
    if (BookmarkManager.INSTANCE.isAsyncBookmarksLoadingInProgress())
    {
      mSavedInstanceState = savedInstanceState;
      updateLoadingPlaceholder(view, true);
      return;
    }

    super.onViewCreated(view, savedInstanceState);
    onViewCreatedInternal(view);
  }

  private void onViewCreatedInternal(@NonNull View view)
  {
    configureBookmarksListAdapter();

    configureFab(view);

    requireActivity().addMenuProvider(mMenuProvider, getViewLifecycleOwner());

    ActionBar bar = ((AppCompatActivity) requireActivity()).getSupportActionBar();
    if (bar != null)
      bar.setTitle(mCategoryDataSource.getData().getName());

    ViewGroup toolbar = requireActivity().findViewById(R.id.toolbar);
    mSearchContainer = toolbar.findViewById(R.id.search_container);
    UiUtils.hide(mSearchContainer, R.id.back);

    mToolbarController = new BookmarksToolbarController(toolbar, requireActivity(), this);
    mToolbarController.setHint(R.string.search_in_the_list);

    configureRecyclerAnimations();
    configureRecyclerDividers();

    // recycler view already has an InsetListener in BaseMwmRecyclerFragment
    // here we must reset it, because the logic is different from a common use case
    ViewCompat.setOnApplyWindowInsetsListener(
        getRecyclerView(), new WindowInsetUtils.ScrollableContentInsetsListener(getRecyclerView(), mFabViewOnMap));

    updateLoadingPlaceholder(view, false);
  }

  @Override
  public void onStart()
  {
    super.onStart();
    SearchEngine.INSTANCE.addBookmarkListener(this);
    BookmarkManager.INSTANCE.addLoadingListener(this);
    BookmarkManager.INSTANCE.addSortingListener(this);
    BookmarkManager.INSTANCE.addSharingListener(this);
  }

  @Override
  public void onResume()
  {
    super.onResume();
    if (BookmarkManager.INSTANCE.isAsyncBookmarksLoadingInProgress())
      return;

    BookmarkListAdapter adapter = getBookmarkListAdapter();
    adapter.notifyDataSetChanged();
    updateSorting();
    updateSearchVisibility();
    updateRecyclerVisibility();
  }

  @Override
  public void onPause()
  {
    super.onPause();
  }

  @Override
  public void onStop()
  {
    super.onStop();
    SearchEngine.INSTANCE.removeBookmarkListener(this);
    BookmarkManager.INSTANCE.removeLoadingListener(this);
    BookmarkManager.INSTANCE.removeSortingListener(this);
    BookmarkManager.INSTANCE.removeSharingListener(this);
  }

  private void configureBookmarksListAdapter()
  {
    BookmarkListAdapter adapter = getBookmarkListAdapter();
    adapter.registerAdapterDataObserver(mCategoryDataSource);
    adapter.setOnClickListener((v, position) -> onItemClick(position));
    adapter.setOnLongClickListener((v, position) -> onItemMore(position));
    adapter.setMoreListener((v, position) -> onItemMore(position));
    adapter.setIconClickListener(this::showColorDialog);
  }

  private void configureFab(@NonNull View view)
  {
    mFabViewOnMap = view.findViewById(R.id.show_on_map_fab);
    mFabViewOnMap.setOnClickListener(v -> {
      final Intent i = makeMwmActivityIntent();
      i.putExtra(MwmActivity.EXTRA_CATEGORY_ID, mCategoryDataSource.getData().getId());
      i.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
      startActivity(i);
    });
  }

  private void configureRecyclerAnimations()
  {
    RecyclerView.ItemAnimator itemAnimator = getRecyclerView().getItemAnimator();
    if (itemAnimator != null)
      ((SimpleItemAnimator) itemAnimator).setSupportsChangeAnimations(false);
  }

  private void configureRecyclerDividers()
  {
    RecyclerView.ItemDecoration decorWithPadding = new DividerItemDecorationWithPadding(requireContext());
    getRecyclerView().addItemDecoration(decorWithPadding);
    getRecyclerView().addOnScrollListener(mRecyclerListener);
  }

  private void updateRecyclerVisibility()
  {
    if (isEmptySearchResults())
    {
      requirePlaceholder().setContent(R.string.search_not_found, R.string.search_not_found_query);
    }
    else if (isEmpty())
    {
      requirePlaceholder().setContent(R.string.bookmarks_empty_list_title, R.string.bookmarks_empty_list_message);
    }

    boolean isEmptyRecycler = isEmpty() || isEmptySearchResults();

    showPlaceholder(isEmptyRecycler);

    getBookmarkCollectionAdapter().show(!getBookmarkListAdapter().isSearchResults());

    UiUtils.showIf(!isEmptyRecycler, getRecyclerView(), mFabViewOnMap);

    requireActivity().invalidateOptionsMenu();
  }

  private void updateSearchVisibility()
  {
    if (isEmpty())
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
    if (SearchEngine.INSTANCE.searchInBookmarks(query, mCategoryDataSource.getData().getId(), mLastQueryTimestamp))
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
    BookmarkListAdapter adapter = getBookmarkListAdapter();
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

    BookmarkListAdapter adapter = getBookmarkListAdapter();
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

    BookmarkListAdapter adapter = getBookmarkListAdapter();
    adapter.setSortedResults(null);
    adapter.notifyDataSetChanged();

    updateSortingProgressBar();
  }

  @Override
  public void onSort(@BookmarkCategory.SortingType int sortingType)
  {
    mLastSortTimestamp = System.nanoTime();

    final Location loc = MwmApplication.from(requireContext()).getLocationHelper().getSavedLocation();
    final boolean hasMyPosition = loc != null;
    if (!hasMyPosition && sortingType == BookmarkCategory.SortingType.BY_DISTANCE)
      return;

    final BookmarkCategory category = mCategoryDataSource.getData();
    final double lat = hasMyPosition ? loc.getLatitude() : 0;
    final double lon = hasMyPosition ? loc.getLongitude() : 0;

    category.setLastSortingType(sortingType);
    BookmarkManager.INSTANCE.getSortedCategory(category.getId(), sortingType, hasMyPosition, lat, lon,
                                               mLastSortTimestamp);

    updateSortingProgressBar();
  }

  @NonNull
  private BookmarkListAdapter getBookmarkListAdapter()
  {
    return (BookmarkListAdapter) getAdapter().getAdapters().get(INDEX_BOOKMARKS_LIST_ADAPTER);
  }

  @NonNull
  private BookmarkCollectionAdapter getBookmarkCollectionAdapter()
  {
    return (BookmarkCollectionAdapter) getAdapter().getAdapters().get(INDEX_BOOKMARKS_COLLECTION_ADAPTER);
  }

  @Override
  public void onResetSorting()
  {
    mLastSortTimestamp = 0;
    mCategoryDataSource.getData().resetLastSortingType();

    BookmarkListAdapter adapter = getBookmarkListAdapter();
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

    if (!mCategoryDataSource.getData().hasLastSortingType())
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
    BookmarkListAdapter adapter = getBookmarkListAdapter();
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
  @BookmarkCategory.SortingType
  private int[] getAvailableSortingTypes()
  {
    final Location loc = MwmApplication.from(requireContext()).getLocationHelper().getSavedLocation();
    final boolean hasMyPosition = loc != null;
    return mCategoryDataSource.getData().getAvailableSortingTypes(hasMyPosition);
  }

  private int getLastSortingType()
  {
    final BookmarkCategory category = mCategoryDataSource.getData();
    if (category.hasLastSortingType())
      return category.getLastSortingType();
    return -1;
  }

  private int getLastAvailableSortingType()
  {
    int currentType = getLastSortingType();
    @BookmarkCategory.SortingType
    int[] types = getAvailableSortingTypes();
    for (@BookmarkCategory.SortingType int type : types)
    {
      if (type == currentType)
        return currentType;
    }
    return -1;
  }

  private boolean isEmpty()
  {
    return !getBookmarkListAdapter().isSearchResults() && getBookmarkListAdapter().getItemCount() == 0;
  }

  private boolean isEmptySearchResults()
  {
    return getBookmarkListAdapter().isSearchResults() && getBookmarkListAdapter().getItemCount() == 0;
  }

  private boolean isLastOwnedCategory()
  {
    return BookmarkManager.INSTANCE.getCategoriesCount() == 1;
  }

  private void updateSortingProgressBar()
  {
    requireActivity().invalidateOptionsMenu();
  }

  public void onItemClick(int position)
  {
    final Intent intent = makeMwmActivityIntent();

    BookmarkListAdapter adapter = getBookmarkListAdapter();

    switch (adapter.getItemViewType(position))
    {
    case BookmarkListAdapter.TYPE_SECTION, BookmarkListAdapter.TYPE_DESC ->
    {
      return;
    }
    case BookmarkListAdapter.TYPE_BOOKMARK -> onBookmarkClicked(position, intent, adapter);
    case BookmarkListAdapter.TYPE_TRACK -> onTrackClicked(position, intent, adapter);
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
    i.putExtra(MwmActivity.EXTRA_CATEGORY_ID, track.getCategoryId());
    i.putExtra(MwmActivity.EXTRA_TRACK_ID, track.getTrackId());
  }

  private void onBookmarkClicked(int position, @NonNull Intent i, @NonNull BookmarkListAdapter adapter)
  {
    final BookmarkInfo bookmark = (BookmarkInfo) adapter.getItem(position);
    i.putExtra(MwmActivity.EXTRA_CATEGORY_ID, bookmark.getCategoryId());
    i.putExtra(MwmActivity.EXTRA_BOOKMARK_ID, bookmark.getBookmarkId());
  }

  private void showColorDialog(int position)
  {
    BookmarkListAdapter adapter = getBookmarkListAdapter();

    final Object item = adapter.getItem(position);
    if (item == null)
      return;
    mSelectedItemType = adapter.getItemViewType(position);

    final Bundle args = new Bundle();
    if (mSelectedItemType == BookmarkListAdapter.TYPE_TRACK)
    {
      final Track track = (Track) item;
      mSelectedItemId = track.getTrackId();
      args.putInt(BookmarkColorDialogFragment.ICON_COLOR, PredefinedColors.getPredefinedColorIndex(track.getColor()));
    }
    else if (mSelectedItemType == BookmarkListAdapter.TYPE_BOOKMARK)
    {
      final BookmarkInfo bookmark = (BookmarkInfo) item;
      mSelectedItemId = bookmark.getBookmarkId();
      args.putInt(BookmarkColorDialogFragment.ICON_COLOR, bookmark.getIcon().getColor());
      args.putInt(BookmarkColorDialogFragment.ICON_RES, bookmark.getIcon().getResId());
    }

    final BookmarkColorDialogFragment dialogFragment = new BookmarkColorDialogFragment();
    dialogFragment.setArguments(args);
    dialogFragment.show(getChildFragmentManager(), null);
  }

  @Override
  public void onBookmarkColorSet(int colorPos)
  {
    if (mSelectedItemId == -1)
      return;

    final BookmarkListAdapter adapter = getBookmarkListAdapter();
    final int position = adapter.getPositionById(mSelectedItemId, mSelectedItemType);
    if (position == -1)
      return;

    final Object item = adapter.getItem(position);
    if (item == null)
      return;

    if (mSelectedItemType == BookmarkListAdapter.TYPE_TRACK)
    {
      final Track track = (Track) item;
      final int from = track.getColor();
      final int to = PredefinedColors.getColor(colorPos);
      if (from == to)
        return;
      track.setColor(to);
    }
    else if (mSelectedItemType == BookmarkListAdapter.TYPE_BOOKMARK)
    {
      final BookmarkInfo bookmark = (BookmarkInfo) item;
      final int from = bookmark.getIcon().getColor();
      final int to = PredefinedColors.getColor(colorPos);
      if (from == to)
        return;
      final int colorIndex = PredefinedColors.getPredefinedColorIndex(to);
      if (colorIndex == -1)
        return;
      final Icon newIcon = new Icon(colorIndex, bookmark.getIcon().getType());
      bookmark.update(bookmark.getName(), newIcon, bookmark.getDescription());
    }

    adapter.notifyItemChanged(position);

    mSelectedItemId = -1;
    mSelectedItemType = -1;
  }

  public void onItemMore(int position)
  {
    BookmarkListAdapter adapter = getBookmarkListAdapter();

    final Object item = adapter.getItem(position);
    if (item == null)
      return;
    mSelectedItemType = adapter.getItemViewType(position);

    switch (mSelectedItemType)
    {
    case BookmarkListAdapter.TYPE_SECTION:
    case BookmarkListAdapter.TYPE_DESC:
      // Do nothing here?
      break;

    case BookmarkListAdapter.TYPE_BOOKMARK:
      final BookmarkInfo bookmark = (BookmarkInfo) item;
      mSelectedItemId = bookmark.getBookmarkId();
      MenuBottomSheetFragment.newInstance(BOOKMARKS_MENU_ID, bookmark.getName())
          .show(getChildFragmentManager(), BOOKMARKS_MENU_ID);
      break;

    case BookmarkListAdapter.TYPE_TRACK:
      final Track track = (Track) item;
      mSelectedItemId = track.getTrackId();
      MenuBottomSheetFragment.newInstance(TRACK_MENU_ID, track.getName())
          .show(getChildFragmentManager(), TRACK_MENU_ID);
      break;
    }
  }

  private void onDeleteTrackSelected(long trackId)
  {
    BookmarkManager.INSTANCE.deleteTrack(trackId);
    getBookmarkListAdapter().notifyDataSetChanged();
  }

  private void onShareActionSelected()
  {
    if (mSelectedItemId == -1)
      return;
    final BookmarkInfo info = BookmarkManager.INSTANCE.getBookmarkInfo(mSelectedItemId);
    if (info == null)
      return;
    SharingUtils.shareBookmark(requireContext(), info);
  }

  private void onEditActionSelected()
  {
    if (mSelectedItemId == -1)
      return;
    final BookmarkInfo info = BookmarkManager.INSTANCE.getBookmarkInfo(mSelectedItemId);
    if (info == null)
      return;
    BookmarkListAdapter adapter = getBookmarkListAdapter();
    EditBookmarkFragment.editBookmark(info.getCategoryId(), info.getBookmarkId(), requireActivity(),
                                      getChildFragmentManager(), (bookmarkId, movedFromCategory) -> {
                                        if (movedFromCategory)
                                          resetSearchAndSort();
                                        else
                                          adapter.notifyDataSetChanged();
                                      });
  }

  private void onTrackEditActionSelected()
  {
    if (mSelectedItemId == -1)
      return;
    final Track track = BookmarkManager.INSTANCE.getTrack(mSelectedItemId);
    EditBookmarkFragment.editTrack(track.getCategoryId(), track.getTrackId(), requireActivity(),
                                   getChildFragmentManager(), (trackId, movedFromCategory) -> {
                                     if (movedFromCategory)
                                       resetSearchAndSort();
                                     else
                                       getBookmarkListAdapter().notifyDataSetChanged();
                                   });
  }

  private void onDeleteActionSelected()
  {
    if (mSelectedItemId == -1)
      return;
    BookmarkManager.INSTANCE.deleteBookmark(mSelectedItemId);
    getBookmarkListAdapter().notifyDataSetChanged();
    if (mSearchMode)
      mNeedUpdateSorting = true;
    updateSearchVisibility();
    updateRecyclerVisibility();
  }

  private void onSortOptionSelected()
  {
    ChooseBookmarksSortingTypeFragment.chooseSortingType(getAvailableSortingTypes(), getLastSortingType(),
                                                         requireActivity(), getChildFragmentManager());
  }

  private void onShareOptionSelected(FileType fileType)
  {
    long catId = mCategoryDataSource.getData().getId();
    BookmarksSharingHelper.INSTANCE.prepareBookmarkCategoryForSharing(requireActivity(), catId, fileType);
  }

  private void onSettingsOptionSelected()
  {
    BookmarkCategorySettingsActivity.startForResult(this, startBookmarkSettingsForResult,
                                                    mCategoryDataSource.getData());
  }

  private void onDeleteOptionSelected()
  {
    requireActivity().setResult(Activity.RESULT_OK);
    requireActivity().finish();
  }

  private ArrayList<MenuBottomSheetItem> getOptionsMenuItems()
  {
    @BookmarkCategory.SortingType
    int[] types = getAvailableSortingTypes();
    ArrayList<MenuBottomSheetItem> items = new ArrayList<>();
    if (!isEmpty())
    {
      if (types.length > 0)
        items.add(new MenuBottomSheetItem(R.string.sort, R.drawable.ic_sort, this::onSortOptionSelected));
      items.add(new MenuBottomSheetItem(R.string.export_file, R.drawable.ic_file_kmz,
                                        () -> onShareOptionSelected(FileType.Kml)));
      items.add(new MenuBottomSheetItem(R.string.export_file_gpx, R.drawable.ic_file_gpx,
                                        () -> onShareOptionSelected(FileType.Gpx)));
      items.add(new MenuBottomSheetItem(R.string.export_file_geojson, R.drawable.ic_file_geojson,
                                        () -> onShareOptionSelected(FileType.GeoJson)));
    }
    items.add(new MenuBottomSheetItem(R.string.edit, R.drawable.ic_settings, this::onSettingsOptionSelected));
    if (!isLastOwnedCategory())
      items.add(new MenuBottomSheetItem(R.string.delete_list, R.drawable.ic_delete, this::onDeleteOptionSelected));
    return items;
  }

  private ArrayList<MenuBottomSheetItem> getBookmarkMenuItems()
  {
    ArrayList<MenuBottomSheetItem> items = new ArrayList<>();
    items.add(new MenuBottomSheetItem(R.string.share, R.drawable.ic_share, this::onShareActionSelected));
    items.add(new MenuBottomSheetItem(R.string.edit, R.drawable.ic_edit, this::onEditActionSelected));
    items.add(new MenuBottomSheetItem(R.string.delete, R.drawable.ic_delete, this::onDeleteActionSelected));
    return items;
  }

  private ArrayList<MenuBottomSheetItem> getTrackMenuItems(final Track track)
  {
    ArrayList<MenuBottomSheetItem> items = new ArrayList<>();
    items.add(new MenuBottomSheetItem(R.string.edit, R.drawable.ic_edit, this::onTrackEditActionSelected));
    items.add(new MenuBottomSheetItem(R.string.export_file, R.drawable.ic_file_kmz,
                                      () -> onShareTrackSelected(track.getTrackId(), FileType.Kml)));
    items.add(new MenuBottomSheetItem(R.string.export_file_gpx, R.drawable.ic_file_gpx,
                                      () -> onShareTrackSelected(track.getTrackId(), FileType.Gpx)));
    items.add(new MenuBottomSheetItem(R.string.export_file_geojson, R.drawable.ic_file_geojson,
                                      () -> onShareTrackSelected(track.getTrackId(), FileType.GeoJson)));
    items.add(new MenuBottomSheetItem(R.string.delete, R.drawable.ic_delete,
                                      () -> onDeleteTrackSelected(track.getTrackId())));
    return items;
  }

  private void onShareTrackSelected(long trackId, FileType fileType)
  {
    BookmarksSharingHelper.INSTANCE.prepareTrackForSharing(requireActivity(), trackId, fileType);
  }

  @Override
  public void onPreparedFileForSharing(@NonNull BookmarkSharingResult result)
  {
    BookmarksSharingHelper.INSTANCE.onPreparedFileForSharing(requireActivity(), shareLauncher, result);
  }

  private void handleActivityResult()
  {
    getBookmarkListAdapter().notifyDataSetChanged();
    ActionBar actionBar = ((AppCompatActivity) requireActivity()).getSupportActionBar();
    actionBar.setTitle(mCategoryDataSource.getData().getName());
  }

  @Override
  public void onBookmarksLoadingFinished()
  {
    View view = getView();
    if (view == null)
      return;

    super.onViewCreated(view, mSavedInstanceState);
    onViewCreatedInternal(view);
    updateRecyclerVisibility();
    updateLoadingPlaceholder(view, false);
  }

  private void updateLoadingPlaceholder(@NonNull View root, boolean isShowLoadingPlaceholder)
  {
    View loadingPlaceholder = root.findViewById(R.id.placeholder_loading);
    UiUtils.showIf(!isShowLoadingPlaceholder, root, R.id.show_on_map_fab);
    UiUtils.showIf(isShowLoadingPlaceholder, loadingPlaceholder);
  }

  @Override
  @Nullable
  public ArrayList<MenuBottomSheetItem> getMenuBottomSheetItems(@NonNull String id)
  {
    if (id.equals(BOOKMARKS_MENU_ID))
      return getBookmarkMenuItems();
    if (id.equals(TRACK_MENU_ID))
    {
      if (mSelectedItemId == -1)
        return null;
      final Track track = BookmarkManager.INSTANCE.getTrack(mSelectedItemId);
      return getTrackMenuItems(track);
    }
    if (id.equals(OPTIONS_MENU_ID))
      return getOptionsMenuItems();
    return null;
  }
}
