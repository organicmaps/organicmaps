package app.organicmaps.search;

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.location.Location;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.annotation.CallSuper;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentManager;
import androidx.lifecycle.Observer;
import androidx.lifecycle.ViewModelProvider;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import androidx.viewpager.widget.ViewPager;
import app.organicmaps.MwmApplication;
import app.organicmaps.R;
import app.organicmaps.downloader.CountrySuggestFragment;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.sdk.downloader.MapManager;
import app.organicmaps.sdk.location.LocationListener;
import app.organicmaps.sdk.routing.RouteMarkType;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.search.SearchEngine;
import app.organicmaps.sdk.search.SearchListener;
import app.organicmaps.sdk.search.SearchRecents;
import app.organicmaps.sdk.search.SearchResult;
import app.organicmaps.sdk.util.Config;
import app.organicmaps.sdk.util.Language;
import app.organicmaps.sdk.util.SharedPropertiesUtils;
import app.organicmaps.util.UiUtils;
import app.organicmaps.widget.PlaceholderView;
import app.organicmaps.widget.SearchShimmerView;
import app.organicmaps.widget.SearchToolbarController;
import com.google.android.material.bottomsheet.BottomSheetBehavior;
import com.google.android.material.tabs.TabLayout;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class SearchFragment extends Fragment implements SearchListener, CategoriesAdapter.CategoriesUiListener
{
  @NonNull
  private final List<HiddenCommand> mHiddenCommands = new ArrayList<>();
  private final List<RecyclerView> mAttachedRecyclers = new ArrayList<>();
  private final LastPosition mLastPosition = new LastPosition();
  private SearchFragmentListener mSearchFragmentListener;
  private View mResultsFrame;
  @Nullable
  private RecyclerView mResults;
  private int mNavH = 0;
  private int mExpandedOffset = 0;
  private View mTabFrame;
  private View mAppBar;
  private PlaceholderView mResultsPlaceholder;
  private SearchShimmerView mShimmerView;
  private SearchPageViewModel mSearchViewModel;

  // Debouncer for runSearch() — collapses bursts of keystrokes into a single engine invocation.
  // searchInteractive() fans out to both SearchInViewport + EverywhereSearch internally, so the
  // saving doubles for the per-prefix cost. ~200 ms matches the Material Design autocomplete guidance.
  private static final long SEARCH_DEBOUNCE_MS = 200;
  private final Handler mSearchDebounceHandler = new Handler(Looper.getMainLooper());
  private final Runnable mDebouncedRunSearch = this::runSearch;

  // Last (hasQuery, activeTab) snapshot applied by syncNestedScrollingState(); both null until the
  // first call. Used to skip the repeat work / requestLayout() when neither input changed.
  @Nullable
  private Boolean mNestedScrollingSyncedHasQuery;
  @Nullable
  private Integer mNestedScrollingSyncedActiveTab;

  @Nullable
  private TabAdapter mTabAdapter;
  private ViewPager mPager;
  private TabLayout mTabLayout;
  @Nullable
  private WindowInsetsCompat mLastKnownInsets = null;

  @NonNull
  private SearchToolbarController mToolbarController;
  private final RecyclerView.OnScrollListener mRecyclerListener = new RecyclerView.OnScrollListener() {
    @Override
    public void onScrollStateChanged(@NonNull RecyclerView recyclerView, int newState)
    {
      if (newState == RecyclerView.SCROLL_STATE_DRAGGING)
        mToolbarController.deactivate();
    }
  };
  private final ActivityResultLauncher<Intent> startVoiceRecognitionForResult =
      registerForActivityResult(new ActivityResultContracts.StartActivityForResult(),
                                activityResult -> { mToolbarController.onVoiceRecognitionResult(activityResult); });
  @Nullable
  private Boolean mSearchPreviouslyEnabled;
  private final Observer<Boolean> mSearchEnabledObserver = new Observer<>() {
    public void onChanged(Boolean enabled)
    {
      if (enabled == null)
        return;

      final boolean wasEnabled = Boolean.TRUE.equals(mSearchPreviouslyEnabled);
      mSearchPreviouslyEnabled = enabled;

      if (!enabled)
      {
        if (!wasEnabled)
          return; // spurious disable — sheet was already closed, leave foreign searches alone

        if (mToolbarController.hasQuery())
          mToolbarController.clear();
        // cancel() → cancelAllSearches() already resets the engine's stored query.
        SearchEngine.INSTANCE.cancel();
        return;
      }

      final SearchRequest request = mSearchViewModel.getPendingRequest();
      final String query = request != null ? request.query : null;
      if (query == null || query.isEmpty())
        return;

      mSearchAdapter.clear();
      stopSearch();

      // setQuery() fires the text watcher, which schedules the debounced search; runSearch() consumes
      // the pending request (locale). When the query already matches the toolbar the watcher won't
      // fire, so go through the debouncer directly to keep the timing consistent.
      if (query.equals(getQuery()))
        runSearchDebounced();
      else
        setQuery(query, request.isCategory);
    }
  };
  private final Observer<Integer> mBottomSheetStateObserver = new Observer<>() {
    public void onChanged(Integer state)
    {
      if (state == null)
        return;

      // The sheet just became visible — instantiate the History/Categories pager lazily.
      if (state != BottomSheetBehavior.STATE_HIDDEN)
        setupTabsIfNeeded();

      if (state != BottomSheetBehavior.STATE_EXPANDED)
        mToolbarController.deactivate();
      else if (!mToolbarController.hasQuery())
        activateToolbar();
    }
  };
  @SuppressWarnings("NullableProblems")
  @NonNull
  private SearchAdapter mSearchAdapter;
  private final LocationListener mLocationListener = new LocationListener() {
    @Override
    public void onLocationUpdated(@NonNull Location location)
    {
      mLastPosition.set(location.getLatitude(), location.getLongitude());
    }
  };
  private boolean mSearchRunning;

  private static boolean doShowDownloadSuggest()
  {
    return (MapManager.nativeGetDownloadedCount() == 0 && !MapManager.nativeIsDownloading());
  }

  private void showDownloadSuggest()
  {
    final FragmentManager fm = getChildFragmentManager();
    final String fragmentName = CountrySuggestFragment.class.getName();
    Fragment fragment = fm.findFragmentByTag(fragmentName);

    if (fragment == null || fragment.isDetached() || fragment.isRemoving())
    {
      fragment = fm.getFragmentFactory().instantiate(requireActivity().getClassLoader(), fragmentName);
      fm.beginTransaction().add(R.id.download_suggest_frame, fragment, fragmentName).commit();
    }
  }

  private void hideDownloadSuggest()
  {
    if (!isAdded())
      return;

    final FragmentManager manager = getChildFragmentManager();
    final Fragment fragment = manager.findFragmentByTag(CountrySuggestFragment.class.getName());
    if (fragment != null && !fragment.isDetached() && !fragment.isRemoving())
      manager.beginTransaction().remove(fragment).commitAllowingStateLoss();
  }

  private void updateFrames()
  {
    final boolean hasQuery = mToolbarController.hasQuery();

    UiUtils.showIf(hasQuery, mResultsFrame);
    UiUtils.showIf(!hasQuery, mTabFrame);
    UiUtils.showIf(!hasQuery, mPager);
    if (hasQuery)
      hideDownloadSuggest();
    else if (doShowDownloadSuggest())
      showDownloadSuggest();
    else
      hideDownloadSuggest();
    syncNestedScrollingState();
    updatePeekHeight();
  }

  private void updatePeekHeight()
  {
    if (mAppBar == null)
      return;

    mAppBar.post(() -> {
      int height = mAppBar.getHeight();
      if (!mToolbarController.hasQuery() && mTabFrame.getVisibility() == View.VISIBLE)
        height += mTabFrame.getHeight();
      mSearchViewModel.setToolbarHeight(height);
    });
  }

  private void updateResultsPlaceholder()
  {
    final boolean show = !mSearchRunning && mSearchAdapter.getItemCount() == 0 && mToolbarController.hasQuery();

    UiUtils.showIf(show, mResultsPlaceholder);
  }

  private void hideShimmer()
  {
    if (mShimmerView != null)
    {
      mShimmerView.stopShimmer();
      UiUtils.hide(mShimmerView);
    }
  }

  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState)
  {
    return inflater.inflate(R.layout.fragment_search, container, false);
  }

  @Override
  public void onAttach(@NonNull Context context)
  {
    super.onAttach(context);
    Fragment parent = getParentFragment();
    if (!(parent instanceof SearchFragmentListener))
      throw new IllegalStateException(parent + " must implement SearchFragmentListener");
    mSearchFragmentListener = (SearchFragmentListener) parent;
  }

  @Override
  public void onDetach()
  {
    mSearchFragmentListener = null;
    super.onDetach();
  }

  @CallSuper
  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    mSearchAdapter = new SearchAdapter(this);
    mSearchViewModel = new ViewModelProvider(requireActivity()).get(SearchPageViewModel.class);

    ViewGroup root = (ViewGroup) view;
    mPager = root.findViewById(R.id.pages);

    mToolbarController = new ToolbarController(view);
    mTabLayout = root.findViewById(R.id.tabs);
    mTabFrame = root.findViewById(R.id.tab_frame);
    mResultsFrame = root.findViewById(R.id.results_frame);
    mResults = mResultsFrame.findViewById(R.id.recycler);
    setRecyclerScrollListener(mResults);
    ViewCompat.setOnApplyWindowInsetsListener(mResults, (v, insets) -> {
      mNavH = insets.getInsets(WindowInsetsCompat.Type.navigationBars()).bottom;
      updateAllRecyclerBottomPadding();
      return insets;
    });
    mSearchViewModel.getExpandedOffset().observe(getViewLifecycleOwner(), offset -> {
      mExpandedOffset = offset != null ? offset : 0;
      updateAllRecyclerBottomPadding();
    });
    mResultsPlaceholder = mResultsFrame.findViewById(R.id.placeholder);
    mResultsPlaceholder.setContent(R.string.search_not_found, R.string.search_not_found_query);
    mShimmerView = mResultsFrame.findViewById(R.id.search_shimmer);
    mSearchAdapter.registerAdapterDataObserver(new RecyclerView.AdapterDataObserver() {
      @Override
      public void onChanged()
      {
        updateResultsPlaceholder();
      }
    });

    mResults.setLayoutManager(new LinearLayoutManager(view.getContext()));
    mResults.setAdapter(mSearchAdapter);

    mPager.setClipToPadding(false);
    // Store insets and dispatch to tab fragment RecyclerViews.
    // Padding the ViewPager itself is wrong — it shortens the tab fragment layout
    // without giving those RecyclerViews their own scrollable bottom padding.
    ViewCompat.setOnApplyWindowInsetsListener(mPager, (v, insets) -> {
      mLastKnownInsets = insets;
      dispatchInsetsToTabFragments(insets);
      return insets;
    });

    final SearchResult[] cachedResults = SearchEngine.INSTANCE.getCachedResults();
    if (cachedResults != null)
    {
      final String cachedQuery = SearchEngine.INSTANCE.getCachedSearchBarQuery();
      if (!TextUtils.isEmpty(cachedQuery))
        mToolbarController.setQuerySilently(cachedQuery, false);
      mSearchAdapter.refreshData(cachedResults);
      mSearchRunning = false;
      mToolbarController.showProgress(false);
      updateFrames();
      updateResultsPlaceholder();
      // Cached results already satisfy any pending restore request — consume it so the
      // mSearchEnabledObserver doesn't wipe the adapter and re-fire a fresh search.
      mSearchViewModel.clearPendingRequest();
    }

    mSearchViewModel.getSearchPageLastState().observe(getViewLifecycleOwner(), mBottomSheetStateObserver);

    if (Config.isSearchHistoryEnabled())
      mTabLayout.setVisibility(View.VISIBLE);
    else
      mTabLayout.setVisibility(View.GONE);
    mAppBar = root.findViewById(R.id.app_bar);

    updateFrames();
    SearchEngine.INSTANCE.addListener(this);

    // Pre-warm tabs after the activity's critical path so the first sheet open is instant.
    // Idempotent — if the sheet opens before the post runs, setupTabsIfNeeded() fires from the
    // bottom-sheet observer instead and this no-ops.
    view.post(this::setupTabsIfNeeded);
  }

  private void setupTabsIfNeeded()
  {
    if (mTabAdapter != null || getView() == null)
      return;

    final ViewPager pager = mPager;
    final TabAdapter tabAdapter = new TabAdapter(getChildFragmentManager(), pager, mTabLayout);
    mTabAdapter = tabAdapter;
    pager.setOffscreenPageLimit(tabAdapter.getCount());
    pager.addOnPageChangeListener(new ViewPager.SimpleOnPageChangeListener() {
      @Override
      public void onPageSelected(int position)
      {
        updateNestedScrollingForTab(tabAdapter, position);
      }
    });

    final SharedPreferences preferences = MwmApplication.prefs(requireContext());
    final int lastSelectedTabPosition = preferences.getInt(Config.KEY_PREF_LAST_SEARCHED_TAB, 0);
    if (SearchRecents.getSize() == 0 && Config.isSearchHistoryEnabled())
      pager.setCurrentItem(lastSelectedTabPosition);

    tabAdapter.setTabSelectedListener(tab -> {
      mToolbarController.deactivate();
      mSearchViewModel.notifyHistoryChanged();
    });
    pager.post(() -> updateNestedScrollingForTab(tabAdapter, pager.getCurrentItem()));

    // The tab fragments missed the initial inset dispatch — replay it now.
    if (mLastKnownInsets != null)
      dispatchInsetsToTabFragments(mLastKnownInsets);
  }

  @Override
  public void onViewStateRestored(@Nullable Bundle savedInstanceState)
  {
    super.onViewStateRestored(savedInstanceState);
    // Android restores the EditText text here via setText(), which fires our TextWatcher and
    // schedules a debounced runSearch — redundant on rotation when cached results are still valid.
    // Cancel the pending debounce so the recreated fragment doesn't run the same search again.
    if (savedInstanceState != null && SearchEngine.INSTANCE.getCachedResults() != null)
      mSearchDebounceHandler.removeCallbacks(mDebouncedRunSearch);
  }

  @Override
  public void onStart()
  {
    super.onStart();
    mToolbarController.attach(requireActivity());
    mSearchViewModel.getSearchEnabled().observe(getViewLifecycleOwner(), mSearchEnabledObserver);
  }

  @Override
  public void onResume()
  {
    super.onResume();
    MwmApplication.from(requireContext()).getLocationHelper().addListener(mLocationListener);

    // onPause() stops the shimmer; if we are resuming mid-search with no results yet, restore it
    // so the results pane isn't blank until the next results callback arrives.
    if (mSearchRunning && mSearchAdapter.getItemCount() == 0 && mShimmerView != null)
    {
      UiUtils.show(mShimmerView);
      mShimmerView.startShimmer();
    }
  }

  @Override
  public void onPause()
  {
    MwmApplication.from(requireContext()).getLocationHelper().removeListener(mLocationListener);
    hideShimmer();
    super.onPause();
  }

  @Override
  public void onStop()
  {
    super.onStop();
    mToolbarController.detach();
  }

  @Override
  public void onDestroyView()
  {
    mSearchDebounceHandler.removeCallbacks(mDebouncedRunSearch);
    for (RecyclerView v : mAttachedRecyclers)
      v.removeOnScrollListener(mRecyclerListener);
    mAttachedRecyclers.clear();
    SearchEngine.INSTANCE.removeListener(this);
    super.onDestroyView();
  }

  private String getQuery()
  {
    return mToolbarController.getQuery();
  }

  private boolean isCategory()
  {
    return mToolbarController.isCategory();
  }

  void setQuery(String text, boolean isCategory)
  {
    mToolbarController.setQuery(text, isCategory);
  }

  private boolean tryRecognizeHiddenCommand(@NonNull String query)
  {
    for (HiddenCommand command : getHiddenCommands())
    {
      if (command.execute(query))
        return true;
    }

    return false;
  }

  @NonNull
  private List<HiddenCommand> getHiddenCommands()
  {
    if (mHiddenCommands.isEmpty())
    {
      mHiddenCommands.addAll(Arrays.asList(
          new BadStorageCommand("?emulateBadStorage", requireContext()), new JavaCrashCommand("?emulateJavaCrash"),
          new NativeCrashCommand("?emulateNativeCrash"), new PushTokenCommand("?pushToken")));
    }

    return mHiddenCommands;
  }

  void showSingleResultOnMap(@NonNull SearchResult result, int resultIndex)
  {
    final String query = getQuery();
    if (Config.isSearchHistoryEnabled())
    {
      SearchRecents.add(query, requireContext());
      mSearchViewModel.notifyHistoryChanged();
    }
    SearchEngine.INSTANCE.setQuery(query);

    if (RoutingController.get().isWaitingPoiPick())
    {
      if (RoutingController.get().isPoiPickReplaceStop()
          || RoutingController.get().getWaitingPoiPickType() == RouteMarkType.Intermediate)
        SearchEngine.INSTANCE.showResult(resultIndex);
      else
      {
        final String subtitle = (result.description != null) ? result.description.localizedFeatureType : "";
        final String title = TextUtils.isEmpty(result.name) ? subtitle : result.name;

        final MapObject point = MapObject.createMapObject(MapObject.SEARCH, title, subtitle, result.lat, result.lon);
        RoutingController.get().onPoiSelected(point);
        mSearchFragmentListener.closeSearch();
      }
    }
    else
    {
      SearchEngine.INSTANCE.selectResult(resultIndex);
    }

    mToolbarController.deactivate();
  }

  private void onSearchEnd()
  {
    if (mSearchRunning && isAdded())
      updateSearchView();
  }

  private void updateSearchView()
  {
    mSearchRunning = false;
    mToolbarController.showProgress(false);
    hideShimmer();
    updateFrames();
    updateResultsPlaceholder();
  }

  private void stopSearch()
  {
    mSearchDebounceHandler.removeCallbacks(mDebouncedRunSearch);
    SearchEngine.INSTANCE.cancel();
    updateSearchView();
  }

  private void runSearchDebounced()
  {
    mSearchDebounceHandler.removeCallbacks(mDebouncedRunSearch);
    mSearchDebounceHandler.postDelayed(mDebouncedRunSearch, SEARCH_DEBOUNCE_MS);
  }

  private void runSearch()
  {
    // The previous search should be cancelled before the new one is started, since previous search
    // results are no longer needed.
    SearchEngine.INSTANCE.cancel();

    boolean hasLocation = mLastPosition.valid;
    double lat = mLastPosition.lat;
    double lon = mLastPosition.lon;

    // Fall back to the last known location if the fragment's listener hasn't received a fix yet.
    if (!hasLocation)
    {
      final Location saved = MwmApplication.from(requireContext()).getLocationHelper().getSavedLocation();
      if (saved != null)
      {
        hasLocation = true;
        lat = saved.getLatitude();
        lon = saved.getLongitude();
      }
    }

    final SearchRequest request = mSearchViewModel.getPendingRequest();
    // Locale applies only to this initial query; consume the request so later manual edits fall
    // back to the keyboard locale.
    String locale =
        (request != null && request.locale != null) ? request.locale : Language.getKeyboardLocale(requireContext());
    mSearchViewModel.clearPendingRequest();

    SearchEngine.INSTANCE.setQuery(getQuery());
    boolean started = SearchEngine.INSTANCE.searchInteractive(getQuery(), isCategory(), locale, System.nanoTime(),
                                                              true /* isMapAndTable */, hasLocation, lat, lon);
    if (!started)
    {
      stopSearch();
      return;
    }

    mSearchRunning = true;
    mToolbarController.showProgress(true);
    updateResultsPlaceholder();

    if (mSearchAdapter.getItemCount() == 0)
    {
      UiUtils.show(mShimmerView);
      mShimmerView.startShimmer();
    }

    updateFrames();
  }

  @Override
  public void onResultsUpdate(@NonNull SearchResult[] results, long timestamp)
  {
    if (!isAdded() || !mToolbarController.hasQuery())
      return;

    refreshSearchResults(results);
  }

  @Override
  public void onResultsEnd(long timestamp)
  {
    onSearchEnd();
  }

  @Override
  public void onSearchCategorySelected(@Nullable String category)
  {
    if (Config.isSearchHistoryEnabled() && category != null)
    {
      SearchRecents.add(category.trim(), requireContext());
      mSearchViewModel.notifyHistoryChanged();
    }
    mToolbarController.setQuery(category, true);
  }

  private void refreshSearchResults(@NonNull SearchResult[] results)
  {
    mSearchRunning = true;
    hideShimmer();
    updateFrames();
    mSearchAdapter.refreshData(results);
    mToolbarController.showProgress(true);
  }

  public boolean onBackPressed()
  {
    if (mToolbarController.hasQuery())
    {
      mToolbarController.clear();
      return true;
    }

    mToolbarController.deactivate();
    if (RoutingController.get().isWaitingPoiPick())
    {
      RoutingController.get().onPoiSelected(null);
    }

    return false;
  }

  public void setRecyclerScrollListener(RecyclerView recycler)
  {
    recycler.addOnScrollListener(mRecyclerListener);
    mAttachedRecyclers.add(recycler);
    if (mTabAdapter != null)
      updateNestedScrollingForTab(mTabAdapter, mPager.getCurrentItem());
  }

  private void updateNestedScrollingForTab(@NonNull TabAdapter tabAdapter, int selectedPosition)
  {
    // Replay the last known insets to any tab fragment views that are now attached.
    // This handles the timing gap where ViewPager creates fragment views lazily,
    // after the initial window-insets dispatch has already completed.
    if (mLastKnownInsets != null)
      dispatchInsetsToTabFragments(mLastKnownInsets);

    updateAllRecyclerBottomPadding();

    syncNestedScrollingState();
  }

  /**
   * Ensures exactly one RecyclerView participates in nested scrolling at any time so
   * BottomSheetBehavior.findScrollingChild() always resolves to the visible list.
   *
   * - In search mode (hasQuery): enable results RV, disable all tab RVs.
   * - In tab mode (!hasQuery): disable results RV, enable only the active tab's RV.
   *
   * After updating flags, requestLayout() is called on the bottom sheet so
   * BottomSheetBehavior.onLayoutChild() re-runs findScrollingChild() and refreshes
   * its internal nestedScrollingChildRef before the next touch gesture.
   */
  private void syncNestedScrollingState()
  {
    final boolean hasQuery = mToolbarController.hasQuery();
    final int activeTab = mPager.getCurrentItem();

    // updateFrames() runs on every keystroke and every results batch — but the nested-scrolling
    // flags only flip on hasQuery / activeTab transitions, so cache the last pair and skip the
    // sheet requestLayout() when nothing changed.
    if (mNestedScrollingSyncedHasQuery != null && mNestedScrollingSyncedActiveTab != null
        && hasQuery == mNestedScrollingSyncedHasQuery && activeTab == mNestedScrollingSyncedActiveTab)
      return;
    mNestedScrollingSyncedHasQuery = hasQuery;
    mNestedScrollingSyncedActiveTab = activeTab;

    if (mResults != null)
      ViewCompat.setNestedScrollingEnabled(mResults, hasQuery);

    if (mTabAdapter != null)
    {
      for (int i = 0; i < mTabAdapter.getCount(); i++)
      {
        Fragment f = mTabAdapter.getItem(i);
        if (f == null || f.getView() == null)
          continue;
        RecyclerView rv = f.getView().findViewById(R.id.recycler);
        if (rv != null)
          ViewCompat.setNestedScrollingEnabled(rv, !hasQuery && i == activeTab);
      }
    }

    View bottomSheet = getBottomSheetContainer();
    if (bottomSheet != null)
      bottomSheet.requestLayout();
  }

  private void updateAllRecyclerBottomPadding()
  {
    int padding = mNavH + mExpandedOffset;
    if (mResults != null)
      mResults.setPadding(mResults.getPaddingLeft(), mResults.getPaddingTop(), mResults.getPaddingRight(), padding);
    if (mTabAdapter == null)
      return;
    for (int i = 0; i < mTabAdapter.getCount(); i++)
    {
      Fragment f = mTabAdapter.getItem(i);
      if (f == null || f.getView() == null)
        continue;
      RecyclerView rv = f.getView().findViewById(R.id.recycler);
      if (rv != null)
        rv.setPadding(rv.getPaddingLeft(), rv.getPaddingTop(), rv.getPaddingRight(), padding);
    }
  }

  private void dispatchInsetsToTabFragments(@NonNull WindowInsetsCompat insets)
  {
    if (mTabAdapter == null)
      return;
    for (int i = 0; i < mTabAdapter.getCount(); i++)
    {
      Fragment f = mTabAdapter.getItem(i);
      if (f != null && f.getView() != null)
        ViewCompat.dispatchApplyWindowInsets(f.getView(), insets);
    }
  }

  /**
   * Returns the BottomSheet container view (search_page_container) that has
   * the BottomSheetBehavior attached, or null if not found.
   */
  @Nullable
  private View getBottomSheetContainer()
  {
    View view = getView();
    if (view == null)
      return null;
    ViewGroup parent = (ViewGroup) view.getParent();
    while (parent != null)
    {
      if (parent.getId() == R.id.search_page_container)
        return parent;
      if (parent.getParent() instanceof ViewGroup)
        parent = (ViewGroup) parent.getParent();
      else
        break;
    }
    return null;
  }

  @NonNull
  public SearchToolbarController requireController()
  {
    return mToolbarController;
  }

  public void activateToolbar()
  {
    mToolbarController.activate();
  }

  interface SearchFragmentListener
  {
    void onSearchClicked();
    void closeSearch();
  }

  private static class LastPosition
  {
    double lat;
    double lon;
    boolean valid;

    public void set(double lat, double lon)
    {
      this.lat = lat;
      this.lon = lon;
      valid = true;
    }
  }

  private static class BadStorageCommand extends HiddenCommand.BaseHiddenCommand
  {
    @NonNull
    Context mContext;

    BadStorageCommand(@NonNull String command, @NonNull Context context)
    {
      super(command);
      mContext = context;
    }

    @Override
    void executeInternal()
    {
      SharedPropertiesUtils.setShouldShowEmulateBadStorageSetting(true);
    }
  }

  private static class JavaCrashCommand extends HiddenCommand.BaseHiddenCommand
  {
    JavaCrashCommand(@NonNull String command)
    {
      super(command);
    }

    @Override
    void executeInternal()
    {
      throw new RuntimeException("Diagnostic java crash!");
    }
  }

  private static class NativeCrashCommand extends HiddenCommand.BaseHiddenCommand
  {
    NativeCrashCommand(@NonNull String command)
    {
      super(command);
    }

    @Override
    void executeInternal()
    {
      Framework.nativeMakeCrash();
    }
  }

  private static class PushTokenCommand extends HiddenCommand.BaseHiddenCommand
  {
    PushTokenCommand(@NonNull String command)
    {
      super(command);
    }

    @Override
    void executeInternal()
    {}
  }

  private class ToolbarController extends SearchToolbarController
  {
    public ToolbarController(View root)
    {
      super(root, SearchFragment.this.requireActivity());
      ViewCompat.setOnApplyWindowInsetsListener(getToolbar(), null);
      root.findViewById(R.id.close_search).setOnClickListener(v -> mSearchFragmentListener.closeSearch());
    }

    @Override
    public void setQuery(CharSequence query)
    {
      super.setQuery(query);
      if (!TextUtils.isEmpty(query))
        mSearchFragmentListener.onSearchClicked();
    }

    @Override
    protected void onTextChanged(String query)
    {
      if (!isAdded())
        return;

      mSearchViewModel.setCurrentToolbarCategorical(isCategory());

      if (query.trim().isEmpty())
      {
        mSearchAdapter.clear();
        stopSearch();
        return;
      }

      if (tryRecognizeHiddenCommand(query))
      {
        mSearchAdapter.clear();
        stopSearch();
        requireActivity().onBackPressed();
        return;
      }

      runSearchDebounced();
    }

    @Override
    protected boolean onStartSearchClick()
    {
      if (Config.isSearchHistoryEnabled())
      {
        SearchRecents.add(getQuery(), requireContext());
        mSearchViewModel.notifyHistoryChanged();
      }
      deactivate();
      mSearchFragmentListener.onSearchClicked();
      return true;
    }

    @Override
    protected int getVoiceInputPrompt()
    {
      return R.string.search_map;
    }

    @Override
    protected void startVoiceRecognition(Intent intent)
    {
      startVoiceRecognitionForResult.launch(intent);
    }

    @Override
    protected boolean supportsVoiceSearch()
    {
      return true;
    }

    @Override
    public void onUpClick()
    {
      requireActivity().onBackPressed();
    }
  }
}
