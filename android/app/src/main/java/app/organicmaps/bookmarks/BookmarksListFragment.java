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
import java.util.LinkedHashSet;
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
  private static final String EXTRA_SELECTED_ITEM_ID = "selected_item_id";
  private static final String EXTRA_SELECTED_ITEM_TYPE = "selected_item_type";
  private static final String EXTRA_SELECTION_MODE = "selection_mode";
  private static final String EXTRA_SELECTED_BOOKMARK_IDS = "selected_bookmark_ids";
  private static final String EXTRA_SELECTED_TRACK_IDS = "selected_track_ids";
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
  private long mSelectedItemId = -1;
  private int mSelectedItemType = -1;
  private boolean mSelectionMode = false;
  // LinkedHashSet: the first selected bookmark — or track, if no bookmark is selected — defines the color the
  // picker opens with.
  @NonNull
  private final Set<Long> mSelectedBookmarkIds = new LinkedHashSet<>();
  @NonNull
  private final Set<Long> mSelectedTrackIds = new LinkedHashSet<>();
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
      if (mSelectionMode)
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
      if (mSelectionMode)
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

    if (savedInstanceState != null)
    {
      mSelectedItemId = savedInstanceState.getLong(EXTRA_SELECTED_ITEM_ID, -1);
      mSelectedItemType = savedInstanceState.getInt(EXTRA_SELECTED_ITEM_TYPE, -1);
      mSelectionMode = savedInstanceState.getBoolean(EXTRA_SELECTION_MODE, false);
      addAll(mSelectedBookmarkIds, savedInstanceState.getLongArray(EXTRA_SELECTED_BOOKMARK_IDS));
      addAll(mSelectedTrackIds, savedInstanceState.getLongArray(EXTRA_SELECTED_TRACK_IDS));
    }

    shareLauncher = SharingUtils.RegisterLauncher(this);
  }

  @Override
  public void onSaveInstanceState(@NonNull Bundle outState)
  {
    super.onSaveInstanceState(outState);
    outState.putLong(EXTRA_SELECTED_ITEM_ID, mSelectedItemId);
    outState.putInt(EXTRA_SELECTED_ITEM_TYPE, mSelectedItemType);
    outState.putBoolean(EXTRA_SELECTION_MODE, mSelectionMode);
    outState.putLongArray(EXTRA_SELECTED_BOOKMARK_IDS, toLongArray(mSelectedBookmarkIds));
    outState.putLongArray(EXTRA_SELECTED_TRACK_IDS, toLongArray(mSelectedTrackIds));
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

  private static void addAll(@NonNull Set<Long> ids, @Nullable long[] values)
  {
    if (values == null)
      return;
    for (long value : values)
      ids.add(value);
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

    mBackCallback = new OnBackPressedCallback(mSelectionMode || mSearchMode) {
      @Override
      public void handleOnBackPressed()
      {
        // Both modes take over the toolbar, so back leaves the mode before it leaves the screen.
        if (mSelectionMode)
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

    ActionBar bar = ((AppCompatActivity) requireActivity()).getSupportActionBar();
    if (bar != null)
      bar.setTitle(mCategoryDataSource.getData().getName());

    ViewGroup toolbar = requireActivity().findViewById(R.id.toolbar);
    mSearchContainer = toolbar.findViewById(R.id.search_container);
    UiUtils.hide(mSearchContainer, R.id.back);

    mToolbarController = new BookmarksToolbarController(toolbar, requireActivity(), this);
    mToolbarController.setHint(R.string.search_in_the_list);

    // Restores the contextual toolbar after a configuration change.
    if (mSelectionMode)
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

    BookmarkListAdapter adapter = getBookmarkListAdapter();
    if (mSelectionMode)
    {
      // The list can be changed from the outside (place page, editor, another list screen).
      adapter.refreshDataSource();
      pruneSelection();
      updateSelectionUi();
    }

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
    view.findViewById(R.id.selection_action_color).setOnClickListener(v -> onChangeSelectionColorSelected());
    view.findViewById(R.id.selection_action_move).setOnClickListener(v -> onMoveSelectionSelected());
    view.findViewById(R.id.selection_action_delete).setOnClickListener(v -> onDeleteSelectionSelected());
  }

  /**
   * Slides the contextual actions in and out. The bar stays INVISIBLE rather than GONE, so that its height is
   * always measured: both the slide offset and the recycler's bottom inset are derived from it.
   */
  private void updateSelectionActions()
  {
    // Tracked instead of read back from the view: the bar stays VISIBLE until the slide-out ends, so the view
    // would report the state it is leaving and the pending end action would never be cancelled.
    if (mSelectionMode != mSelectionActionsShown)
    {
      mSelectionActionsShown = mSelectionMode;
      mSelectionActions.animate().cancel();
      if (mSelectionMode)
      {
        mSelectionActions.setTranslationY(getSelectionActionsOffset());
        mSelectionActions.setVisibility(View.VISIBLE);
        mSelectionActions.animate().translationY(0).setDuration(SELECTION_ACTIONS_ANIMATION_MS).start();
      }
      else
      {
        mSelectionActions.animate()
            .translationY(getSelectionActionsOffset())
            .setDuration(SELECTION_ACTIONS_ANIMATION_MS)
            .withEndAction(() -> mSelectionActions.setVisibility(View.INVISIBLE))
            .start();
      }
    }

    final boolean hasSelection = getSelectedCount() > 0;
    setActionEnabled(mSelectionActions.findViewById(R.id.selection_action_color), hasSelection);
    setActionEnabled(mSelectionActions.findViewById(R.id.selection_action_move), hasSelection);
    setActionEnabled(mSelectionActions.findViewById(R.id.selection_action_delete), hasSelection);
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
    ViewCompat.setOnApplyWindowInsetsListener(getRecyclerView(),
                                              mSelectionMode ? mSelectionInsetsListener : mFabInsetsListener);
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

    getBookmarkCollectionAdapter().show(!mSelectionMode && !getBookmarkListAdapter().isSearchResults());

    UiUtils.showIf(!isEmptyRecycler, getRecyclerView());
    UiUtils.showIf(!isEmptyRecycler && !mSelectionMode, mFabViewOnMap);
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
      mBackCallback.setEnabled(mSelectionMode || mSearchMode);
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
    if (mSelectionMode)
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
    mSelectedItemType = adapter.getItemViewType(position);

    if (mSelectedItemType == BookmarkListAdapter.TYPE_TRACK)
    {
      final Track track = (Track) item;
      mSelectedItemId = track.getTrackId();
      ColorPickerFragment.show(getChildFragmentManager(), track.getColor());
    }
    else if (mSelectedItemType == BookmarkListAdapter.TYPE_BOOKMARK)
    {
      final BookmarkInfo bookmark = (BookmarkInfo) item;
      mSelectedItemId = bookmark.getBookmarkId();
      ColorPickerFragment.show(getChildFragmentManager(), bookmark.getIcon().argb());
    }
  }

  @Override
  public void onColorSet(@ColorInt int color)
  {
    if (mSelectionMode)
    {
      applyColorToSelection(color);
      return;
    }

    if (mSelectedItemId == -1)
      return;

    final BookmarkListAdapter adapter = getBookmarkListAdapter();
    final int position = adapter.getPositionById(mSelectedItemId, mSelectedItemType);
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

    mSelectedItemId = -1;
    mSelectedItemType = -1;
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

  private int getSelectedTrackPosition()
  {
    if (mSelectedItemId == -1 || mSelectedItemType != BookmarkListAdapter.TYPE_TRACK)
      return -1;
    return getBookmarkListAdapter().getPositionById(mSelectedItemId, BookmarkListAdapter.TYPE_TRACK);
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
    final long trackId = mSelectedItemId;
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
    EditBookmarkFragment.editBookmark(info.getCategoryId(), info.getBookmarkId(), getChildFragmentManager());
  }

  private void onTrackEditActionSelected()
  {
    if (mSelectedItemId == -1)
      return;
    final Track track = BookmarkManager.INSTANCE.getTrack(mSelectedItemId);
    EditBookmarkFragment.editTrack(track.getCategoryId(), track.getTrackId(), getChildFragmentManager());
  }

  private void onDeleteActionSelected()
  {
    if (mSelectedItemId == -1)
      return;
    final long bookmarkId = mSelectedItemId;
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
    mSelectedItemId = -1;
    mSelectedItemType = -1;
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
    return mSelectionMode;
  }

  @Override
  public boolean isSelected(int itemType, long itemId)
  {
    // Bookmark and track ids are numbered independently.
    if (itemType == BookmarkListAdapter.TYPE_BOOKMARK)
      return mSelectedBookmarkIds.contains(itemId);
    if (itemType == BookmarkListAdapter.TYPE_TRACK)
      return mSelectedTrackIds.contains(itemId);
    return false;
  }

  private int getSelectedCount()
  {
    return mSelectedBookmarkIds.size() + mSelectedTrackIds.size();
  }

  private void setSelectionMode(boolean enabled)
  {
    if (mSelectionMode == enabled)
      return;
    mSelectionMode = enabled;
    mSelectedBookmarkIds.clear();
    mSelectedTrackIds.clear();
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
    return selectableCount > 0 && getSelectedCount() == selectableCount;
  }

  private void setAllSelected(boolean selected)
  {
    mSelectedBookmarkIds.clear();
    mSelectedTrackIds.clear();

    final BookmarkListAdapter adapter = getBookmarkListAdapter();
    if (selected)
      adapter.collectSelectableIds(mSelectedBookmarkIds, mSelectedTrackIds);

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

    final Set<Long> ids = adapter.getItemViewType(position) == BookmarkListAdapter.TYPE_BOOKMARK ? mSelectedBookmarkIds
                                                                                                 : mSelectedTrackIds;
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
    if (!mSelectedBookmarkIds.isEmpty())
      retainAll(mSelectedBookmarkIds, category.getBookmarkIds());
    if (!mSelectedTrackIds.isEmpty())
      retainAll(mSelectedTrackIds, category.getTrackIds());
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
    if (mSelectionMode)
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
    // Insets first: the slide offset is derived from the bar's bottom margin, which the inset pass sets.
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
      final int count = getSelectedCount();
      bar.setTitle(mSelectionMode ? getResources().getQuantityString(R.plurals.n_items_selected, count, count)
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
    final int count = getSelectedCount();
    if (count == 0)
      return;
    final String title = getResources().getQuantityString(R.plurals.delete_n_items_dialog_title, count, count);
    DeleteConfirmationDialogFragment.showDialog(getChildFragmentManager(), title, DELETE_SELECTED_REQUEST_KEY);
  }

  private void onDeleteSelectionConfirmed()
  {
    if (!mSelectionMode || getSelectedCount() == 0)
      return;

    final long[] bookmarkIds = toLongArray(mSelectedBookmarkIds);
    final long[] trackIds = toLongArray(mSelectedTrackIds);
    forgetSelectedItems();
    BookmarkManager.INSTANCE.deleteBookmarksAndTracks(bookmarkIds, trackIds);
    finishBatchOperation();
  }

  private void onMoveSelectionSelected()
  {
    if (getSelectedCount() == 0)
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
    if (!mSelectionMode || getSelectedCount() == 0)
      return;

    // The chooser preselects the current list and can hand it back.
    if (newCategory.getId() == mCategoryDataSource.getData().getId())
    {
      exitSelectionMode();
      return;
    }

    final long[] bookmarkIds = toLongArray(mSelectedBookmarkIds);
    final long[] trackIds = toLongArray(mSelectedTrackIds);
    forgetSelectedItems();
    BookmarkManager.INSTANCE.moveBookmarksAndTracks(bookmarkIds, trackIds, newCategory.getId());
    finishBatchOperation();
  }

  private void onChangeSelectionColorSelected()
  {
    if (getSelectedCount() == 0)
      return;
    ColorPickerFragment.show(getChildFragmentManager(), getInitialSelectionColor());
  }

  @ColorInt
  private int getInitialSelectionColor()
  {
    for (long bookmarkId : mSelectedBookmarkIds)
    {
      final BookmarkInfo bookmark = BookmarkManager.INSTANCE.getBookmarkInfo(bookmarkId);
      if (bookmark != null)
        return bookmark.getIcon().argb();
    }
    for (long trackId : mSelectedTrackIds)
    {
      if (BookmarkManager.INSTANCE.hasTrack(trackId))
        return BookmarkManager.INSTANCE.getTrack(trackId).getColor();
    }
    return BookmarkManager.INSTANCE.getLastEditedColor();
  }

  private void applyColorToSelection(@ColorInt int color)
  {
    if (!mSelectionMode || getSelectedCount() == 0)
      return;

    BookmarkManager.INSTANCE.changeBookmarksAndTracksColor(toLongArray(mSelectedBookmarkIds),
                                                           toLongArray(mSelectedTrackIds), color);
    finishBatchOperation();
  }

  /**
   * Prunes the adapter's cached search and sorted snapshots, which {@code refreshDataSource()} does not touch.
   * Must run before the items are deleted or moved in the core.
   */
  private void forgetSelectedItems()
  {
    getBookmarkListAdapter().removeDeletedItems(mSelectedBookmarkIds, mSelectedTrackIds);
  }

  /**
   * The common tail of every batch action: selection mode is always left, so the user sees the result.
   */
  private void finishBatchOperation()
  {
    // BookmarkCategory caches the item counts in Java while the ids are read from the core, and after a delete
    // or a move a stale count makes BookmarkListAdapter.getItem() throw.
    getBookmarkListAdapter().refreshDataSource();
    // Also refreshes the list and restores the regular toolbar, the FAB and the child lists section.
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
    getBookmarkListAdapter().notifyDataSetChanged();
    if (mSelectionMode)
    {
      updateSelectionUi();
      return;
    }
    ActionBar actionBar = ((AppCompatActivity) requireActivity()).getSupportActionBar();
    actionBar.setTitle(mCategoryDataSource.getData().getName());
  }

  @Override
  public void onBookmarksLoadingFinished()
  {
    View view = getView();
    if (view == null)
      return;

    // A load can also finish while the screen is already up — an import from another app, say. Re-running the
    // setup would double the menu provider, the decorations and the search controller, so only the data is
    // refreshed: the load can have replaced this very category, when the imported file kept its name.
    if (mViewInitialized)
    {
      final BookmarkListAdapter adapter = getBookmarkListAdapter();
      adapter.refreshDataSource();
      adapter.notifyDataSetChanged();
    }
    else
    {
      super.onViewCreated(view, mSavedInstanceState);
      onViewCreatedInternal(view);
    }
    // Must run before updateRecyclerVisibility(): it shows the FAB unconditionally, which would undo the
    // decision to hide it in selection mode or on an empty list.
    updateLoadingPlaceholder(view, false);
    updateRecyclerVisibility();
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
