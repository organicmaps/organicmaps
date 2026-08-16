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
import androidx.activity.OnBackPressedCallback;
import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.annotation.CallSuper;
import androidx.annotation.ColorInt;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;
import androidx.core.graphics.Insets;
import androidx.core.view.MenuProvider;
import androidx.core.view.OnApplyWindowInsetsListener;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;
import androidx.lifecycle.ViewModelProvider;
import androidx.recyclerview.widget.ConcatAdapter;
import androidx.recyclerview.widget.RecyclerView;
import androidx.recyclerview.widget.SimpleItemAnimator;
import app.organicmaps.MwmActivity;
import app.organicmaps.MwmApplication;
import app.organicmaps.R;
import app.organicmaps.base.BaseMwmRecyclerFragment;
import app.organicmaps.dialog.DeleteConfirmationDialogFragment;
import app.organicmaps.sdk.bookmarks.data.BookmarkCategory;
import app.organicmaps.sdk.bookmarks.data.BookmarkInfo;
import app.organicmaps.sdk.bookmarks.data.BookmarkManager;
import app.organicmaps.sdk.bookmarks.data.CategoryDataSource;
import app.organicmaps.sdk.bookmarks.data.FileType;
import app.organicmaps.sdk.bookmarks.data.Icon;
import app.organicmaps.sdk.bookmarks.data.SortedBlock;
import app.organicmaps.sdk.bookmarks.data.Track;
import app.organicmaps.sdk.search.BookmarkSearchListener;
import app.organicmaps.sdk.search.SearchEngine;
import app.organicmaps.util.SharingUtils;
import app.organicmaps.util.UiUtils;
import app.organicmaps.util.Utils;
import app.organicmaps.util.WindowInsetUtils;
import app.organicmaps.util.bottomsheet.ExportMenuItems;
import app.organicmaps.util.bottomsheet.MenuBottomSheetFragment;
import app.organicmaps.util.bottomsheet.MenuBottomSheetItem;
import app.organicmaps.widget.SearchToolbarController;
import app.organicmaps.widget.colorpicker.ColorPickerFragment;
import app.organicmaps.widget.placepage.EditBookmarkFragment;
import app.organicmaps.widget.recycler.CardSectionDividerDecoration;
import com.google.android.material.floatingactionbutton.ExtendedFloatingActionButton;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Objects;
import java.util.Set;

public class BookmarksListFragment extends BaseMwmRecyclerFragment<ConcatAdapter>
    implements BookmarkManager.BookmarksSortingListener, BookmarkManager.BookmarksLoadingListener,
               BookmarkSearchListener, ChooseBookmarksSortingTypeFragment.ChooseSortingTypeListener,
               MenuBottomSheetFragment.MenuBottomSheetInterface, ColorPickerFragment.OnColorChangeListener,
               ChooseBookmarkCategoryFragment.Listener, BookmarkListAdapter.SelectionStateProvider
{
  public static final String TAG = BookmarksListFragment.class.getSimpleName();
  public static final String EXTRA_CATEGORY = "bookmark_category";
  private static final int INDEX_BOOKMARKS_COLLECTION_ADAPTER = 0;
  private static final int INDEX_BOOKMARKS_LIST_ADAPTER = 1;
  private static final long SELECTION_ACTIONS_ANIMATION_MS = 300;
  private static final String BOOKMARKS_MENU_ID = "BOOKMARKS_MENU_BOTTOM_SHEET";
  private static final String TRACK_MENU_ID = "TRACK_MENU_BOTTOM_SHEET";
  private static final String OPTIONS_MENU_ID = "OPTIONS_MENU_BOTTOM_SHEET";
  private static final String DELETE_SELECTED_REQUEST_KEY = "DeleteSelectedBookmarksConfirmation";

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
  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private BookmarksListViewModel mViewModel;
  @Nullable
  private OnBackPressedCallback mBackCallback;
  private boolean mViewInitialized = false;
  private boolean mSearchMode = false;
  private boolean mNeedUpdateSorting = true;
  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private ViewGroup mSearchContainer;
  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private ExtendedFloatingActionButton mFabViewOnMap;
  private View mSelectionActions;
  private View mSelectionActionColor;
  private View mSelectionActionMove;
  private View mSelectionActionDelete;
  private boolean mSelectionActionsShown = false;
  // Tracks what the Select all / Deselect all title was last built from, so a toggle rebuilds the menu only
  // when that flips.
  private boolean mAllSelected = false;
  private OnApplyWindowInsetsListener mFabInsetsListener;
  private OnApplyWindowInsetsListener mSelectionInsetsListener;
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
      // The contextual actions live in the bottom bar; the toolbar only offers the bulk toggle.
      if (mViewModel.isSelectionMode())
      {
        menu.findItem(R.id.bookmarks_selection_select_all)
            .setTitle(isAllSelected() ? R.string.deselect_all : R.string.select_all);
        return;
      }

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
      if (mViewModel.isSelectionMode())
      {
        menuInflater.inflate(R.menu.menu_bookmarks_selection, menu);
        return;
      }

      menuInflater.inflate(R.menu.option_menu_bookmarks, menu);

      menu.findItem(R.id.bookmarks_search).setVisible(!isEmpty());
    }

    @Override
    public boolean onMenuItemSelected(@NonNull MenuItem menuItem)
    {
      final int itemId = menuItem.getItemId();

      if (itemId == R.id.bookmarks_selection_select_all)
      {
        setAllSelected(!isAllSelected());
        return true;
      }

      if (itemId == R.id.bookmarks_search)
      {
        activateSearch();
        return true;
      }

      if (itemId == R.id.bookmarks_more)
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

    mViewModel = new ViewModelProvider(this).get(BookmarksListViewModel.class);

    // A load that ran while this screen was destroyed renumbered the restored ids. No view to tear down yet.
    final int loadingGeneration = BookmarkManager.INSTANCE.getLoadingGeneration();
    if (mViewModel.isFromEarlierLoad(loadingGeneration))
      mViewModel.dropState();
    mViewModel.bindToLoad(loadingGeneration);

    shareLauncher = SharingUtils.RegisterLauncher(this);
  }

  @NonNull
  private static long[] toLongArray(@NonNull Set<Long> ids)
  {
    final long[] result = new long[ids.size()];
    int i = 0;
    for (long id : ids)
      result[i++] = id;
    return result;
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
    getChildFragmentManager().setFragmentResultListener(
        EditBookmarkFragment.REQUEST_KEY, getViewLifecycleOwner(), (key, result) -> {
          BookmarkListAdapter adapter = getBookmarkListAdapter();
          if (adapter == null)
            return;
          final String action = result.getString(EditBookmarkFragment.RESULT_ACTION);
          if (EditBookmarkFragment.ACTION_DELETED.equals(action))
            resetSearchAndSort();
          else if (EditBookmarkFragment.ACTION_SAVED.equals(action))
          {
            if (result.getBoolean(EditBookmarkFragment.RESULT_MOVED_FROM_CATEGORY, false))
              resetSearchAndSort();
            else
              adapter.notifyDataSetChanged();
          }
        });

    getChildFragmentManager().setFragmentResultListener(DeleteConfirmationDialogFragment.REQUEST_KEY,
                                                        getViewLifecycleOwner(),
                                                        (key, result) -> onDeleteTrackConfirmed());

    getChildFragmentManager().setFragmentResultListener(DELETE_SELECTED_REQUEST_KEY, getViewLifecycleOwner(),
                                                        (key, result) -> onDeleteSelectionConfirmed());

    mBackCallback = new OnBackPressedCallback(mViewModel.isSelectionMode() || mSearchMode) {
      @Override
      public void handleOnBackPressed()
      {
        // Both modes take over the toolbar, so back leaves the mode before it leaves the screen.
        if (mViewModel.isSelectionMode())
          exitSelectionMode();
        else
          resetSearchAndSort();
      }
    };
    requireActivity().getOnBackPressedDispatcher().addCallback(getViewLifecycleOwner(), mBackCallback);

    configureBookmarksListAdapter();

    configureFab(view);
    configureSelectionActions(view);
    createContentInsetsListeners();

    requireActivity().addMenuProvider(mMenuProvider, getViewLifecycleOwner());

    updateToolbarTitle();

    ViewGroup toolbar = requireActivity().findViewById(R.id.toolbar);
    mSearchContainer = toolbar.findViewById(R.id.search_container);
    UiUtils.hide(mSearchContainer, R.id.back);

    mToolbarController = new BookmarksToolbarController(toolbar, requireActivity(), this);
    mToolbarController.setHint(R.string.search_in_the_list);

    // Restores the contextual toolbar after a configuration change.
    if (mViewModel.isSelectionMode())
      updateSelectionUi();

    configureRecyclerAnimations();
    configureRecyclerDividers();
    updateContentInsetsListener();

    updateLoadingPlaceholder(view, false);
    mViewInitialized = true;
  }

  @Override
  public void onDestroyView()
  {
    super.onDestroyView();
    mViewInitialized = false;
  }

  @Override
  public void onStart()
  {
    super.onStart();
    SearchEngine.INSTANCE.addBookmarkListener(this);
    BookmarkManager.INSTANCE.addLoadingListener(this);
    BookmarkManager.INSTANCE.addSortingListener(this);
  }

  @Override
  public void onResume()
  {
    super.onResume();
    if (BookmarkManager.INSTANCE.isAsyncBookmarksLoadingInProgress())
      return;

    // A load that finished between onViewCreated() and onStart(), where the listener is registered, or while the
    // screen was stopped, never reached onBookmarksLoadingFinished(): the view may not even be set up yet, and
    // the ids the screen holds have been renumbered. Take the same path the callback would have taken.
    if (!mViewInitialized || mViewModel.isFromEarlierLoad(BookmarkManager.INSTANCE.getLoadingGeneration()))
    {
      onBookmarksLoadingFinished();
      if (requireActivity().isFinishing())
        return;
    }

    // The list can also be changed from the outside without a reload — the place page, the editor, another list
    // screen — which leaves the ids alone but not the membership.
    if (!refreshFromCore())
      return;

    if (mViewModel.isSelectionMode())
    {
      pruneSelection();
      updateSelectionUi();
    }

    getBookmarkListAdapter().notifyDataSetChanged();
    updateSorting();
    updateSearchVisibility();
    updateRecyclerVisibility();
  }

  @Override
  public void onStop()
  {
    super.onStop();
    SearchEngine.INSTANCE.removeBookmarkListener(this);
    BookmarkManager.INSTANCE.removeLoadingListener(this);
    BookmarkManager.INSTANCE.removeSortingListener(this);
  }

  private void configureBookmarksListAdapter()
  {
    BookmarkListAdapter adapter = getBookmarkListAdapter();
    adapter.registerAdapterDataObserver(mCategoryDataSource);
    adapter.setOnClickListener((v, position) -> onItemClick(position));
    adapter.setOnLongClickListener((v, position) -> onItemLongClick(position));
    adapter.setMoreListener((v, position) -> onItemMore(position));
    adapter.setEyeListener((v, position) -> onToggleTrackVisibilityAt(position));
    adapter.setIconClickListener(this::showColorDialog);
    adapter.setSelectionStateProvider(this);
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

  private void configureSelectionActions(@NonNull View view)
  {
    mSelectionActions = view.findViewById(R.id.selection_actions);
    // The freshly inflated bar is invisible, whatever the mode was before a configuration change.
    mSelectionActionsShown = false;
    // Kept in fields: their enabled state is refreshed on every row toggle.
    mSelectionActionColor = view.findViewById(R.id.selection_action_color);
    mSelectionActionMove = view.findViewById(R.id.selection_action_move);
    mSelectionActionDelete = view.findViewById(R.id.selection_action_delete);
    mSelectionActionColor.setOnClickListener(v -> onChangeSelectionColorSelected());
    mSelectionActionMove.setOnClickListener(v -> onMoveSelectionSelected());
    mSelectionActionDelete.setOnClickListener(v -> onDeleteSelectionSelected());
  }

  /**
   * Slides the contextual actions in and out. The bar stays INVISIBLE rather than GONE, so that its height is
   * always measured: both the slide offset and the recycler's bottom inset are derived from it.
   */
  private void updateSelectionActions()
  {
    // Tracked instead of read back from the view: the bar stays VISIBLE until the slide-out ends, so the view
    // would report the state it is leaving and the pending end action would never be cancelled.
    if (mViewModel.isSelectionMode() != mSelectionActionsShown)
    {
      mSelectionActionsShown = mViewModel.isSelectionMode();
      mSelectionActions.animate().cancel();
      // Before the first layout the bar has no height to slide by, which is the case when selection mode is
      // restored after a configuration change: there it simply belongs on screen from the start.
      final boolean animate = mSelectionActions.isLaidOut();
      if (mViewModel.isSelectionMode())
      {
        mSelectionActions.setTranslationY(animate ? getSelectionActionsOffset() : 0);
        mSelectionActions.setVisibility(View.VISIBLE);
        if (animate)
          mSelectionActions.animate().translationY(0).setDuration(SELECTION_ACTIONS_ANIMATION_MS).start();
      }
      else if (animate)
      {
        mSelectionActions.animate()
            .translationY(getSelectionActionsOffset())
            .setDuration(SELECTION_ACTIONS_ANIMATION_MS)
            .withEndAction(() -> mSelectionActions.setVisibility(View.INVISIBLE))
            .start();
      }
      else
      {
        mSelectionActions.setVisibility(View.INVISIBLE);
      }
    }

    final boolean hasSelection = mViewModel.getSelectedCount() > 0;
    setActionEnabled(mSelectionActionColor, hasSelection);
    setActionEnabled(mSelectionActionMove, hasSelection);
    setActionEnabled(mSelectionActionDelete, hasSelection);
  }

  /**
   * A disabled button stays in the accessibility tree, so a screen reader would announce three dead stops while
   * nothing is selected. Directional focus already skips it, since it requires the view to be enabled.
   */
  private static void setActionEnabled(@NonNull View action, boolean enabled)
  {
    action.setEnabled(enabled);
    action.setImportantForAccessibility(enabled ? View.IMPORTANT_FOR_ACCESSIBILITY_YES
                                                : View.IMPORTANT_FOR_ACCESSIBILITY_NO);
  }

  private float getSelectionActionsOffset()
  {
    final ViewGroup.MarginLayoutParams lp = (ViewGroup.MarginLayoutParams) mSelectionActions.getLayoutParams();
    return mSelectionActions.getHeight() + lp.bottomMargin;
  }

  /**
   * Builds both inset listeners once. Rebuilding them per call would leak: every
   * {@link WindowInsetUtils.ScrollableContentInsetsListener} registers an {@code OnLayoutChangeListener} on its
   * floating bar that is only removed from inside a layout pass, and toggling the bar does not trigger one.
   */
  private void createContentInsetsListeners()
  {
    // recycler view already has an InsetListener in BaseMwmRecyclerFragment
    // here we must reset it, because the logic is different from a common use case
    mFabInsetsListener = wrapContentInsetsListener(mFabViewOnMap);
    mSelectionInsetsListener = wrapContentInsetsListener(mSelectionActions);
  }

  @NonNull
  private OnApplyWindowInsetsListener wrapContentInsetsListener(@NonNull View floatingBar)
  {
    final WindowInsetUtils.ScrollableContentInsetsListener listener =
        new WindowInsetUtils.ScrollableContentInsetsListener(getRecyclerView(), floatingBar);
    return (v, windowInsets) ->
    {
      final WindowInsetsCompat result = listener.onApplyWindowInsets(v, windowInsets);
      updateSelectionActionsSideInsets(windowInsets);
      return result;
    };
  }

  /**
   * Whichever bar floats above the list owns the recycler's bottom inset, and the two never show together.
   */
  private void updateContentInsetsListener()
  {
    ViewCompat.setOnApplyWindowInsetsListener(
        getRecyclerView(), mViewModel.isSelectionMode() ? mSelectionInsetsListener : mFabInsetsListener);
    getRecyclerView().requestApplyInsets();
  }

  /**
   * The shared listener gives a floating bar only its bottom inset, which is enough for the centred
   * wrap_content FAB but not for the full-width action bar: in landscape it would run under a side
   * navigation bar or a display cutout.
   */
  private void updateSelectionActionsSideInsets(@NonNull WindowInsetsCompat windowInsets)
  {
    final Insets insets = windowInsets.getInsets(WindowInsetUtils.TYPE_SAFE_DRAWING);
    final int spacing = getResources().getDimensionPixelSize(R.dimen.margin_base);
    final ViewGroup.MarginLayoutParams lp = (ViewGroup.MarginLayoutParams) mSelectionActions.getLayoutParams();
    final int left = insets.left + spacing;
    final int right = insets.right + spacing;
    if (lp.leftMargin == left && lp.rightMargin == right)
      return;

    lp.leftMargin = left;
    lp.rightMargin = right;
    mSelectionActions.requestLayout();
  }

  private void configureRecyclerAnimations()
  {
    RecyclerView.ItemAnimator itemAnimator = getRecyclerView().getItemAnimator();
    if (itemAnimator != null)
      ((SimpleItemAnimator) itemAnimator).setSupportsChangeAnimations(false);
  }

  private void configureRecyclerDividers()
  {
    getRecyclerView().addItemDecoration(new CardSectionDividerDecoration(requireContext()));
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

    getBookmarkCollectionAdapter().show(!mViewModel.isSelectionMode() && !getBookmarkListAdapter().isSearchResults());

    UiUtils.showIf(!isEmptyRecycler, getRecyclerView());
    UiUtils.showIf(!isEmptyRecycler && !mViewModel.isSelectionMode(), mFabViewOnMap);
    // The FAB is a part of the recycler's bottom inset, so toggling it has to recompute the inset.
    getRecyclerView().requestApplyInsets();

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
    updateBackCallback();
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
    // Leaves an empty field for the next time search is opened: deactivate() only drops focus and the keyboard.
    mToolbarController.clear();
    BookmarkManager.INSTANCE.setNotificationsEnabled(false);
    updateSearchVisibility();
  }

  @Override
  public void onBookmarksSortingCompleted(@NonNull SortedBlock[] sortedBlocks, long timestamp)
  {
    applySortedResults(sortedBlocks, timestamp);
  }

  @Override
  public void onBookmarksSortingCancelled(long timestamp)
  {
    applySortedResults(null, timestamp);
  }

  private void applySortedResults(@Nullable SortedBlock[] sortedBlocks, long timestamp)
  {
    if (mLastSortTimestamp == 0 || mLastSortTimestamp != timestamp)
      return;
    mLastSortTimestamp = 0;

    BookmarkListAdapter adapter = getBookmarkListAdapter();
    adapter.setSortedResults(sortedBlocks);
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

  private void updateBackCallback()
  {
    if (mBackCallback != null)
      mBackCallback.setEnabled(mViewModel.isSelectionMode() || mSearchMode);
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

  /**
   * The adapter always renders the description row, so {@link #isEmpty()} stays false even for a category
   * without a single bookmark or track.
   */
  private boolean hasSelectableItems()
  {
    final BookmarkCategory category = mCategoryDataSource.getData();
    return category.getBookmarksCount() > 0 || category.getTracksCount() > 0;
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
    if (mViewModel.isSelectionMode())
    {
      toggleSelection(position);
      return;
    }

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
    final int itemType = adapter.getItemViewType(position);

    if (itemType == BookmarkListAdapter.TYPE_TRACK)
    {
      final Track track = (Track) item;
      mViewModel.focusItem(track.getTrackId(), itemType);
      ColorPickerFragment.show(getChildFragmentManager(), track.getColor());
    }
    else if (itemType == BookmarkListAdapter.TYPE_BOOKMARK)
    {
      final BookmarkInfo bookmark = (BookmarkInfo) item;
      mViewModel.focusItem(bookmark.getBookmarkId(), itemType);
      ColorPickerFragment.show(getChildFragmentManager(), bookmark.getIcon().argb());
    }
    else
    {
      // A stale position can name a row that has neither, and the previous target must not outlive the tap.
      mViewModel.clearFocusedItem();
    }
  }

  @Override
  public void onColorSet(@ColorInt int color)
  {
    if (mViewModel.isSelectionMode())
    {
      applyColorToSelection(color);
      // The picker was opened for the whole selection, but a row tapped before it can still be focused.
      mViewModel.clearFocusedItem();
      return;
    }

    if (!mViewModel.hasFocusedItem())
      return;

    final BookmarkListAdapter adapter = getBookmarkListAdapter();
    final int position = adapter.getPositionById(mViewModel.getFocusedItemId(), mViewModel.getFocusedItemType());
    if (position != -1)
    {
      final Object item = adapter.getItem(position);
      if (item instanceof Track track)
      {
        if (track.getColor() != color)
        {
          track.setColor(color);
          adapter.notifyItemChanged(position);
        }
      }
      else if (item instanceof BookmarkInfo bookmark)
      {
        if (bookmark.getIcon().argb() != color)
        {
          final Icon newIcon = new Icon(color, bookmark.getIcon().getType());
          bookmark.update(bookmark.getName(), newIcon, bookmark.getDescription());
          adapter.notifyItemChanged(position);
        }
      }
    }

    mViewModel.clearFocusedItem();
  }

  /**
   * A long press is the second way into selection mode; the per-item menu stays on the row's more button.
   */
  private void onItemLongClick(int position)
  {
    if (position == RecyclerView.NO_POSITION)
      return;
    // Selection mode and search are mutually exclusive: the search field covers the contextual toolbar.
    if (mSearchMode)
    {
      onItemMore(position);
      return;
    }
    enterSelectionMode();
    toggleSelection(position);
  }

  public void onItemMore(int position)
  {
    BookmarkListAdapter adapter = getBookmarkListAdapter();
    // notifyDataSetChanged() invalidates holder positions until the next layout pass, so a tap on the more
    // button can arrive with a stale one.
    if (position == RecyclerView.NO_POSITION || position >= adapter.getItemCount())
      return;

    final Object item = adapter.getItem(position);
    if (item == null)
      return;
    final int itemType = adapter.getItemViewType(position);

    switch (itemType)
    {
    case BookmarkListAdapter.TYPE_SECTION:
    case BookmarkListAdapter.TYPE_DESC:
      // No menu to show, but a stale position lands here, and the previous target must not outlive the tap.
      mViewModel.clearFocusedItem();
      break;

    case BookmarkListAdapter.TYPE_BOOKMARK:
      final BookmarkInfo bookmark = (BookmarkInfo) item;
      mViewModel.focusItem(bookmark.getBookmarkId(), itemType);
      MenuBottomSheetFragment.newInstance(BOOKMARKS_MENU_ID, bookmark.getName())
          .show(getChildFragmentManager(), BOOKMARKS_MENU_ID);
      break;

    case BookmarkListAdapter.TYPE_TRACK:
      final Track track = (Track) item;
      mViewModel.focusItem(track.getTrackId(), itemType);
      MenuBottomSheetFragment.newInstance(TRACK_MENU_ID, track.getName())
          .show(getChildFragmentManager(), TRACK_MENU_ID);
      break;
    }
  }

  private int getSelectedTrackPosition()
  {
    if (!mViewModel.hasFocusedItem() || mViewModel.getFocusedItemType() != BookmarkListAdapter.TYPE_TRACK)
      return -1;
    return getBookmarkListAdapter().getPositionById(mViewModel.getFocusedItemId(), BookmarkListAdapter.TYPE_TRACK);
  }

  private void onDeleteTrackSelected()
  {
    final int position = getSelectedTrackPosition();
    if (position == -1)
      return;
    final Track track = (Track) getBookmarkListAdapter().getItem(position);
    DeleteConfirmationDialogFragment.showDialog(getChildFragmentManager(),
                                                getString(R.string.delete_track_dialog_title, track.getName()));
  }

  private void onDeleteTrackConfirmed()
  {
    if (getSelectedTrackPosition() == -1)
      return;
    final long trackId = mViewModel.getFocusedItemId();
    deleteBookmarkListItem(trackId, BookmarkListAdapter.TYPE_TRACK,
                           () -> BookmarkManager.INSTANCE.deleteTrack(trackId));
  }

  private void onToggleTrackVisibilityAt(int position)
  {
    final BookmarkListAdapter adapter = getBookmarkListAdapter();
    if (position == -1 || adapter.getItemViewType(position) != BookmarkListAdapter.TYPE_TRACK)
      return;
    ((Track) adapter.getItem(position)).toggleVisibility();
    adapter.notifyItemChanged(position);
  }

  private void onToggleTrackVisibility(long trackId)
  {
    onToggleTrackVisibilityAt(getBookmarkListAdapter().getPositionById(trackId, BookmarkListAdapter.TYPE_TRACK));
  }

  private void onShareActionSelected()
  {
    if (!mViewModel.hasFocusedItem())
      return;
    final BookmarkInfo info = BookmarkManager.INSTANCE.getBookmarkInfo(mViewModel.getFocusedItemId());
    if (info == null)
      return;
    SharingUtils.shareBookmark(requireContext(), info);
  }

  private void onEditActionSelected()
  {
    if (!mViewModel.hasFocusedItem())
      return;
    final BookmarkInfo info = BookmarkManager.INSTANCE.getBookmarkInfo(mViewModel.getFocusedItemId());
    if (info == null)
      return;
    EditBookmarkFragment.editBookmark(info.getCategoryId(), info.getBookmarkId(), getChildFragmentManager());
  }

  private void onTrackEditActionSelected()
  {
    if (!mViewModel.hasFocusedItem() || !BookmarkManager.INSTANCE.hasTrack(mViewModel.getFocusedItemId()))
      return;
    final Track track = BookmarkManager.INSTANCE.getTrack(mViewModel.getFocusedItemId());
    EditBookmarkFragment.editTrack(track.getCategoryId(), track.getTrackId(), getChildFragmentManager());
  }

  private void onDeleteActionSelected()
  {
    if (!mViewModel.hasFocusedItem())
      return;
    final long bookmarkId = mViewModel.getFocusedItemId();
    deleteBookmarkListItem(bookmarkId, BookmarkListAdapter.TYPE_BOOKMARK,
                           () -> BookmarkManager.INSTANCE.deleteBookmark(bookmarkId));
  }

  private void deleteBookmarkListItem(long itemId, int type, @NonNull Runnable deleteAction)
  {
    final BookmarkListAdapter adapter = getBookmarkListAdapter();
    adapter.removeDeletedItem(itemId, type);
    deleteAction.run();
    adapter.refreshDataSource();
    adapter.notifyDataSetChanged();
    if (mSearchMode)
      mNeedUpdateSorting = true;
    mViewModel.clearFocusedItem();
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
    BookmarksSharingHelper.INSTANCE.prepareBookmarkCategoryForSharing(requireActivity(), shareLauncher, catId,
                                                                      fileType);
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

  @Override
  public boolean isSelectionMode()
  {
    return mViewModel.isSelectionMode();
  }

  @Override
  public boolean isSelected(int itemType, long itemId)
  {
    // Bookmark and track ids are numbered independently.
    if (itemType == BookmarkListAdapter.TYPE_BOOKMARK)
      return mViewModel.getBookmarkIds().contains(itemId);
    if (itemType == BookmarkListAdapter.TYPE_TRACK)
      return mViewModel.getTrackIds().contains(itemId);
    return false;
  }

  private void setSelectionMode(boolean enabled)
  {
    if (!mViewModel.setMode(enabled))
      return;

    getBookmarkListAdapter().notifyDataSetChanged();
    updateSelectionUi();
    updateRecyclerVisibility();
  }

  private void enterSelectionMode()
  {
    setSelectionMode(true);
  }

  private void exitSelectionMode()
  {
    setSelectionMode(false);
  }

  private boolean isAllSelected()
  {
    // Counted per section by the adapter, because search and sorting show a subset of the category.
    final int selectableCount = getBookmarkListAdapter().getSelectableCount();
    return selectableCount > 0 && mViewModel.getSelectedCount() == selectableCount;
  }

  private void setAllSelected(boolean selected)
  {
    mViewModel.clearSelection();

    final BookmarkListAdapter adapter = getBookmarkListAdapter();
    if (selected)
      adapter.collectSelectableIds(mViewModel.getBookmarkIds(), mViewModel.getTrackIds());

    adapter.notifyDataSetChanged();
    updateSelectionCountUi();
  }

  private void toggleSelection(int position)
  {
    final BookmarkListAdapter adapter = getBookmarkListAdapter();
    if (position == RecyclerView.NO_POSITION || position >= adapter.getItemCount())
      return;

    final long itemId = adapter.getItemIdAt(position);
    if (itemId == -1)
      return;

    final Set<Long> ids = adapter.getItemViewType(position) == BookmarkListAdapter.TYPE_BOOKMARK
                            ? mViewModel.getBookmarkIds()
                            : mViewModel.getTrackIds();
    if (!ids.remove(itemId))
      ids.add(itemId);

    adapter.notifyItemChanged(position);
    updateSelectionCountUi();
  }

  /**
   * Drops the ids that no longer belong to this category: an item could be deleted or moved from the outside.
   */
  private void pruneSelection()
  {
    final BookmarkCategory category = mCategoryDataSource.getData();
    if (!mViewModel.getBookmarkIds().isEmpty())
      retainAll(mViewModel.getBookmarkIds(), category.getBookmarkIds());
    if (!mViewModel.getTrackIds().isEmpty())
      retainAll(mViewModel.getTrackIds(), category.getTrackIds());
  }

  private static void retainAll(@NonNull Set<Long> ids, @NonNull long[] existing)
  {
    final Set<Long> keep = new HashSet<>(existing.length);
    for (long id : existing)
      keep.add(id);
    ids.retainAll(keep);
  }

  private void updateSelectionUi()
  {
    final Toolbar toolbar = requireActivity().findViewById(R.id.toolbar);

    // Only the icon and its description are replaced: the navigation click listener belongs to
    // setSupportActionBar, and both it and the system back button end up in the OnBackPressedDispatcher.
    if (mViewModel.isSelectionMode())
    {
      toolbar.setNavigationIcon(R.drawable.ic_close_themed);
      toolbar.setNavigationContentDescription(R.string.cancel);
    }
    else
    {
      UiUtils.showHomeUpButton(toolbar);
      toolbar.setNavigationContentDescription(R.string.back);
    }

    updateBackCallback();
    // Hands the recycler's bottom inset to whichever bar is about to float above it.
    updateContentInsetsListener();
    updateSelectionCountUi();
  }

  /**
   * The part of the selection UI that a single row toggle changes. Kept apart from {@link #updateSelectionUi()},
   * whose rest depends on the mode alone and would otherwise re-register the inset listener on every tap.
   */
  private void updateSelectionCountUi()
  {
    final Activity activity = requireActivity();
    final ActionBar bar = ((AppCompatActivity) activity).getSupportActionBar();
    if (bar != null)
    {
      final int count = mViewModel.getSelectedCount();
      bar.setTitle(mViewModel.isSelectionMode()
                       ? getResources().getQuantityString(R.plurals.n_items_selected, count, count)
                       : mCategoryDataSource.getData().getName());
    }

    updateSelectionActions();

    // Rebuilding the menu re-runs every MenuProvider on the activity, and the only thing a toggle can change
    // there is the Select all / Deselect all title, which flips when the count reaches the list or leaves it.
    final boolean allSelected = isAllSelected();
    if (mAllSelected != allSelected)
    {
      mAllSelected = allSelected;
      activity.invalidateOptionsMenu();
    }
  }

  private void onDeleteSelectionSelected()
  {
    final int count = mViewModel.getSelectedCount();
    if (count == 0)
      return;
    final String title = getResources().getQuantityString(R.plurals.delete_n_items_dialog_title, count, count);
    DeleteConfirmationDialogFragment.showDialog(getChildFragmentManager(), title, DELETE_SELECTED_REQUEST_KEY);
  }

  private void onDeleteSelectionConfirmed()
  {
    if (!mViewModel.isSelectionMode() || mViewModel.getSelectedCount() == 0)
      return;

    final long[] bookmarkIds = toLongArray(mViewModel.getBookmarkIds());
    final long[] trackIds = toLongArray(mViewModel.getTrackIds());
    forgetSelectedItems();
    BookmarkManager.INSTANCE.deleteBookmarksAndTracks(bookmarkIds, trackIds);
    finishMembershipChange();
  }

  private void onMoveSelectionSelected()
  {
    if (mViewModel.getSelectedCount() == 0)
      return;

    final Bundle args = new Bundle();
    // Only preselects a row in the chooser.
    args.putLong(ChooseBookmarkCategoryFragment.CATEGORY_ID, mCategoryDataSource.getData().getId());
    final ChooseBookmarkCategoryFragment fragment = new ChooseBookmarkCategoryFragment();
    fragment.setArguments(args);
    fragment.show(getChildFragmentManager(), null);
  }

  @Override
  public void onCategoryChanged(@NonNull BookmarkCategory newCategory)
  {
    if (!mViewModel.isSelectionMode() || mViewModel.getSelectedCount() == 0)
      return;

    // The chooser preselects the current list and can hand it back. That is a no-op, not a reason to throw away
    // a selection the user built up by hand.
    if (newCategory.getId() == mCategoryDataSource.getData().getId())
      return;

    final long[] bookmarkIds = toLongArray(mViewModel.getBookmarkIds());
    final long[] trackIds = toLongArray(mViewModel.getTrackIds());
    forgetSelectedItems();
    BookmarkManager.INSTANCE.moveBookmarksAndTracks(bookmarkIds, trackIds, newCategory.getId());
    finishMembershipChange();
  }

  private void onChangeSelectionColorSelected()
  {
    if (mViewModel.getSelectedCount() == 0)
      return;
    ColorPickerFragment.show(getChildFragmentManager(), getInitialSelectionColor());
  }

  @ColorInt
  private int getInitialSelectionColor()
  {
    for (long bookmarkId : mViewModel.getBookmarkIds())
    {
      final BookmarkInfo bookmark = BookmarkManager.INSTANCE.getBookmarkInfo(bookmarkId);
      if (bookmark != null)
        return bookmark.getIcon().argb();
    }
    for (long trackId : mViewModel.getTrackIds())
    {
      if (BookmarkManager.INSTANCE.hasTrack(trackId))
        return BookmarkManager.INSTANCE.getTrack(trackId).getColor();
    }
    return BookmarkManager.INSTANCE.getLastEditedColor();
  }

  private void applyColorToSelection(@ColorInt int color)
  {
    if (!mViewModel.isSelectionMode() || mViewModel.getSelectedCount() == 0)
      return;

    BookmarkManager.INSTANCE.changeBookmarksAndTracksColor(toLongArray(mViewModel.getBookmarkIds()),
                                                           toLongArray(mViewModel.getTrackIds()), color);
    // No id and no count changed, so no refresh from the core - but every batch action ends the same way.
    exitSelectionMode();
  }

  /**
   * Prunes the adapter's cached search and sorted snapshots, which {@code refreshDataSource()} does not touch.
   * Must run while the selection is still populated, i.e. before selection mode is left.
   */
  private void forgetSelectedItems()
  {
    getBookmarkListAdapter().removeDeletedItems(mViewModel.getBookmarkIds(), mViewModel.getTrackIds());
  }

  /**
   * The tail of a batch that changed which items this category holds.
   */
  private void finishMembershipChange()
  {
    // Moving items into a child list changes the count on its card, so the child adapter is refreshed too.
    if (!refreshFromCore())
      return;

    // The batch pruned the sorted snapshot by id, but the core is free to have skipped items — a destination
    // list deleted meanwhile, say — so only a new sort can say what this list holds now. A sort in flight is
    // stale for the opposite reason: its results are filtered against the core, and that drops deleted items
    // but not ones moved to another list, which would come back here.
    resortIfSorted();

    // Refreshes the list and restores the regular toolbar, the FAB and the child lists section.
    exitSelectionMode();
  }

  private ArrayList<MenuBottomSheetItem> getOptionsMenuItems()
  {
    @BookmarkCategory.SortingType
    int[] types = getAvailableSortingTypes();
    ArrayList<MenuBottomSheetItem> items = new ArrayList<>();
    if (!isEmpty())
    {
      // Search results are bookmarks only and are reset when search is closed.
      if (!mSearchMode && hasSelectableItems())
        items.add(new MenuBottomSheetItem(R.string.select, R.drawable.ic_check, this::enterSelectionMode));
      if (types.length > 0)
        items.add(new MenuBottomSheetItem(R.string.sort, R.drawable.ic_sort, this::onSortOptionSelected));
      items.addAll(ExportMenuItems.create(this::onShareOptionSelected));
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
    final boolean visible = track.isVisible();
    items.add(new MenuBottomSheetItem(visible ? R.string.hide_track : R.string.show_track,
                                      visible ? R.drawable.ic_hide : R.drawable.ic_show,
                                      () -> onToggleTrackVisibility(track.getTrackId())));
    items.addAll(ExportMenuItems.create(fileType -> onShareTrackSelected(track.getTrackId(), fileType)));
    items.add(new MenuBottomSheetItem(R.string.delete, R.drawable.ic_delete, this::onDeleteTrackSelected));
    return items;
  }

  private void onShareTrackSelected(long trackId, FileType fileType)
  {
    BookmarksSharingHelper.INSTANCE.prepareTrackForSharing(requireActivity(), shareLauncher, trackId, fileType);
  }

  private void handleActivityResult()
  {
    // A result is delivered as soon as the fragment is started, which can be before a load finishes and the
    // adapters exist. onResume() rebuilds everything right after, so there is nothing to refresh here yet.
    if (!mViewInitialized)
      return;

    // The screen that just closed can have renamed this list, moved items in or out of it, or renamed a child.
    if (!refreshFromCore())
      return;

    resortIfSorted();
    getBookmarkListAdapter().notifyDataSetChanged();
    // In selection mode the toolbar shows the count instead of the name, which refreshFromCore() has restored.
    if (mViewModel.isSelectionMode())
      updateSelectionCountUi();
  }

  @Override
  public void onBookmarksLoadingFinished()
  {
    View view = getView();
    if (view == null)
      return;

    // Only records the load; the branches below drop what it invalidated, through paths that redraw with it.
    mViewModel.bindToLoad(BookmarkManager.INSTANCE.getLoadingGeneration());

    // A load rebuilds every category from its file, so the one this screen was opened with can be gone. Nothing
    // below may run then, adapter creation included: every lookup, down to the ids the list is laid out from,
    // goes to the core by category id and aborts there for a group it no longer has.
    if (!hasCategory(mCategoryDataSource.getData().getId()))
    {
      requireActivity().finish();
      return;
    }

    // A load can also finish while the screen is already up — an import from another app, say. Re-running the
    // setup would double the menu provider, the decorations and the search controller, so only the state that
    // comes from the core is rebuilt. The loading placeholder was never shown in that case.
    if (mViewInitialized)
    {
      reloadFromCore();
      return;
    }

    // The parcelled category and everything restored in onCreate() predate the load. No view built from it yet.
    mViewModel.dropState();
    mCategoryDataSource.invalidate();
    super.onViewCreated(view, mSavedInstanceState);
    onViewCreatedInternal(view);
    // Must run before updateRecyclerVisibility(): it shows the FAB unconditionally, which would undo the
    // decision to hide it in selection mode or on an empty list.
    updateLoadingPlaceholder(view, false);
    updateRecyclerVisibility();
  }

  /**
   * Brings the screen back in line with the core after a load replaced the bookmark files behind its back. Every
   * id the screen is holding on to — the selection, the search results, the sorted snapshot, a sort still in
   * flight — was handed out by the previous contents of the file, so all of it is dropped rather than reconciled:
   * a category reloaded from disk renumbers its bookmarks and tracks.
   * <p>
   * The caller has already established that the category is still there.
   */
  private void reloadFromCore()
  {
    if (!refreshFromCore())
      return;

    mViewModel.clearFocusedItem();
    exitSelectionMode();
    // Drops the search and sorted snapshots along with any sort in flight, then re-sorts the new ids.
    resetSearchAndSort();
  }

  /**
   * Re-reads everything the screen caches from the core: the category itself, the item ids the list is laid out
   * from, the child lists and the name. Every path that has to assume the core moved on goes through here.
   *
   * @return false when the category is gone and the screen is closing. No further lookup by its id is allowed
   *     then — see {@link #onBookmarksLoadingFinished()}.
   */
  private boolean refreshFromCore()
  {
    final long categoryId = mCategoryDataSource.getData().getId();
    if (!hasCategory(categoryId))
    {
      requireActivity().finish();
      return false;
    }

    getBookmarkListAdapter().refreshDataSource();
    getBookmarkCollectionAdapter().setCategories(BookmarkManager.INSTANCE.getChildrenCategories(categoryId));
    updateToolbarTitle();
    return true;
  }

  /**
   * A sorted snapshot lists the items the category held when the sort was requested, so anything added or taken
   * away since then is missing from it — or still in it. Only a new sort can tell.
   */
  private void resortIfSorted()
  {
    if (!getBookmarkListAdapter().isSortedResults())
      return;

    forceUpdateSorting();
    // Sorting by distance needs a location that can be gone by now, and no other sort would be started then.
    // Drop the snapshot rather than keep one that no longer matches the category.
    if (mLastSortTimestamp == 0)
      getBookmarkListAdapter().setSortedResults(null);
  }

  private static boolean hasCategory(long categoryId)
  {
    // Reads the cached list rather than asking the core by id, which would abort on a category that is gone.
    for (BookmarkCategory category : BookmarkManager.INSTANCE.getCategories())
      if (category.getId() == categoryId)
        return true;
    return false;
  }

  private void updateToolbarTitle()
  {
    final ActionBar bar = ((AppCompatActivity) requireActivity()).getSupportActionBar();
    if (bar != null)
      bar.setTitle(mCategoryDataSource.getData().getName());
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
      // The sheet restores itself with the focused id, which the core may have dropped since - getTrack() only
      // asserts on that, so in Release it would hand back null.
      if (!mViewModel.hasFocusedItem() || !BookmarkManager.INSTANCE.hasTrack(mViewModel.getFocusedItemId()))
        return null;
      final Track track = BookmarkManager.INSTANCE.getTrack(mViewModel.getFocusedItemId());
      return getTrackMenuItems(track);
    }
    if (id.equals(OPTIONS_MENU_ID))
      return getOptionsMenuItems();
    return null;
  }
}
