package app.organicmaps.search;

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.location.Location;
import android.os.Bundle;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import androidx.activity.OnBackPressedCallback;
import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.annotation.CallSuper;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.view.ViewCompat;
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
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.search.SearchEngine;
import app.organicmaps.sdk.search.SearchListener;
import app.organicmaps.sdk.search.SearchRecents;
import app.organicmaps.sdk.search.SearchResult;
import app.organicmaps.sdk.util.Config;
import app.organicmaps.sdk.util.Language;
import app.organicmaps.sdk.util.SharedPropertiesUtils;
import app.organicmaps.sdk.util.log.Logger;
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
  private long mLastQueryTimestamp;
  private SearchFragmentListener mSearchFragmentListener;
  private View mResultsFrame;
  private View mTabFrame;
  private View mPages;
  private View mAppBar;
  private PlaceholderView mResultsPlaceholder;
  private SearchShimmerView mShimmerView;
  private SearchPageViewModel mSearchViewModel;
  @Nullable
  private TabAdapter mTabAdapter;
  @Nullable
  private ViewPager mPager;

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
  private final OnBackPressedCallback mOnBackPressedCallback = new OnBackPressedCallback(true) {
    @Override
    public void handleOnBackPressed()
    {
      if (!onBackPressed())
      {
        // Temporarily disable this callback and re-dispatch the back press to let activity handle it
        setEnabled(false);
        requireActivity().getOnBackPressedDispatcher().onBackPressed();
        setEnabled(true);
      }
    }
  };
  private final Observer<Boolean> mSearchEnabledObserver = new Observer<>() {
    public void onChanged(Boolean enabled)
    {
      if (enabled == null)
        return;

      if (!enabled)
      {
        if (mToolbarController.hasQuery())
          mToolbarController.clear();
        mSearchViewModel.setSearchQuery(null);
        mSearchViewModel.setLastResults(null);
        SearchEngine.INSTANCE.cancel();
        SearchEngine.INSTANCE.setQuery("");
        return;
      }

      final String query = mSearchViewModel.getSearchQuery();
      if (query == null || query.isEmpty())
        return;
      if (query.equals(getQuery()))
      {
        mSearchViewModel.setSearchQuery(null);
        return;
      }
      setQuery(query, false);
      mSearchViewModel.setSearchQuery(null);
    }
  };
  private final Observer<Integer> mBottomSheetStateObserver = new Observer<>() {
    public void onChanged(Integer state)
    {
      if (state == null)
        return;

      if (state != BottomSheetBehavior.STATE_EXPANDED)
        mToolbarController.deactivate();
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

      if (!TextUtils.isEmpty(getQuery()))
        mSearchAdapter.notifyDataSetChanged();
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
    UiUtils.showIf(!hasQuery, mPages);
    if (hasQuery)
      hideDownloadSuggest();
    else if (doShowDownloadSuggest())
      showDownloadSuggest();
    else
      hideDownloadSuggest();

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

  @CallSuper
  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    mSearchAdapter = new SearchAdapter(this);
    mSearchViewModel = new ViewModelProvider(requireActivity()).get(SearchPageViewModel.class);
    requireActivity().getOnBackPressedDispatcher().addCallback(getViewLifecycleOwner(), mOnBackPressedCallback);

    mSearchFragmentListener = (SearchFragmentListener) getParentFragment();

    ViewGroup root = (ViewGroup) view;
    ViewPager pager = root.findViewById(R.id.pages);
    mPages = pager;
    mPager = pager;

    mToolbarController = new ToolbarController(view);
    TabLayout tabLayout = root.findViewById(R.id.tabs);
    mTabFrame = root.findViewById(R.id.tab_frame);
    mResultsFrame = root.findViewById(R.id.results_frame);
    RecyclerView mResults = mResultsFrame.findViewById(R.id.recycler);
    setRecyclerScrollListener(mResults);
    mResultsPlaceholder = mResultsFrame.findViewById(R.id.placeholder);
    mResultsPlaceholder.setContent(R.string.search_not_found, R.string.search_not_found_query);
    mShimmerView = mResultsFrame.findViewById(R.id.search_shimmer);
    mSearchAdapter.registerAdapterDataObserver(new RecyclerView.AdapterDataObserver()

                                               {
                                                 @Override
                                                 public void onChanged()
                                                 {
                                                   updateResultsPlaceholder();
                                                 }
                                               });

    mResults.setLayoutManager(new LinearLayoutManager(view.getContext()));
    mResults.setAdapter(mSearchAdapter);

    // Restore cached results (query is restored by view state; we only need to repopulate the list).
    final SearchResult[] cachedResults = mSearchViewModel.getLastResults();
    if (cachedResults != null)
    {
      final String cachedQuery = mSearchViewModel.getSearchQuery();
      if (!TextUtils.isEmpty(cachedQuery))
        mToolbarController.setQuerySilently(cachedQuery, false);
      mSearchAdapter.refreshData(cachedResults);
      mSearchRunning = false;
      mToolbarController.showProgress(false);
      updateFrames();
      updateResultsPlaceholder();
    }

    mSearchViewModel.getSearchPageLastState().observe(getViewLifecycleOwner(), mBottomSheetStateObserver);

    if (Config.isSearchHistoryEnabled())
      tabLayout.setVisibility(View.VISIBLE);
    else
      tabLayout.setVisibility(View.GONE);
    mAppBar = root.findViewById(R.id.app_bar);

    final TabAdapter tabAdapter = new TabAdapter(getChildFragmentManager(), pager, tabLayout);
    mTabAdapter = tabAdapter;
    pager.setOffscreenPageLimit(tabAdapter.getCount());
    pager.addOnPageChangeListener(new ViewPager.SimpleOnPageChangeListener() {
      @Override
      public void onPageSelected(int position)
      {
        updateNestedScrollingForTab(tabAdapter, position);
      }
    });

    updateFrames();

    mToolbarController.activate();

    SearchEngine.INSTANCE.addListener(this);

    SharedPreferences preferences = MwmApplication.prefs(requireContext());
    int lastSelectedTabPosition = preferences.getInt(Config.KEY_PREF_LAST_SEARCHED_TAB, 0);
    if (SearchRecents.getSize() == 0 && Config.isSearchHistoryEnabled())
      pager.setCurrentItem(lastSelectedTabPosition);

    tabAdapter.setTabSelectedListener(tab -> {
      mToolbarController.deactivate();
      refreshHistoryFragment();
    });
    pager.post(() -> updateNestedScrollingForTab(tabAdapter, pager.getCurrentItem()));
  }

  @Override
  public void onStart()
  {
    super.onStart();
    mToolbarController.attach(requireActivity());
    mSearchViewModel.getSearchEnabled().observe(getViewLifecycleOwner(), mSearchEnabledObserver);
  }

  public void onResume()
  {
    super.onResume();
    MwmApplication.from(requireContext()).getLocationHelper().addListener(mLocationListener);
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
    mSearchViewModel.setLastResults(mSearchAdapter.getResults());
    mSearchViewModel.setSearchQuery(TextUtils.isEmpty(getQuery()) ? null : getQuery());
    mToolbarController.detach();
    mSearchViewModel.getSearchEnabled().removeObserver(mSearchEnabledObserver);
  }

  @Override
  public void onDestroy()
  {
    for (RecyclerView v : mAttachedRecyclers)
      v.removeOnScrollListener(mRecyclerListener);

    mAttachedRecyclers.clear();
    SearchEngine.INSTANCE.removeListener(this);
    super.onDestroy();
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

  private void refreshHistoryFragment()
  {
    if (mTabAdapter == null)
      return;
    Fragment f = mTabAdapter.getFragmentForTab(TabAdapter.Tab.HISTORY);
    if (f instanceof SearchHistoryFragment)
      ((SearchHistoryFragment) f).refreshHistory();
  }

  void showSingleResultOnMap(@NonNull SearchResult result, int resultIndex)
  {
    final String query = getQuery();
    if (Config.isSearchHistoryEnabled())
    {
      SearchRecents.add(query, requireContext());
      refreshHistoryFragment();
    }
    SearchEngine.INSTANCE.setQuery(query);

    if (RoutingController.get().isWaitingPoiPick())
    {
      final String subtitle = (result.description != null) ? result.description.localizedFeatureType : "";
      final String title = TextUtils.isEmpty(result.name) ? subtitle : result.name;

      final MapObject point = MapObject.createMapObject(MapObject.SEARCH, title, subtitle, result.lat, result.lon);
      RoutingController.get().onPoiSelected(point);
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
    SearchEngine.INSTANCE.cancel();
    updateSearchView();
  }

  private void runSearch()
  {
    // The previous search should be cancelled before the new one is started, since previous search
    // results are no longer needed.
    SearchEngine.INSTANCE.cancel();

    mLastQueryTimestamp = System.nanoTime();

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

    String locale = mSearchViewModel.getInitialLocale();
    if (locale == null)
      locale = Language.getKeyboardLocale(requireContext());
    boolean isMapAndTable = mSearchViewModel.isInitialSearchOnMap();

    SearchEngine.INSTANCE.searchInteractive(getQuery(), isCategory(), locale, mLastQueryTimestamp, isMapAndTable,
                                            hasLocation, lat, lon);

    mSearchViewModel.setInitialLocale(null);
    mSearchViewModel.setInitialSearchOnMap(true);

    mSearchRunning = true;
    mToolbarController.showProgress(true);

    if (SearchShimmerView.isSupported() && mSearchAdapter.getItemCount() == 0)
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
      refreshHistoryFragment();
    }
    mToolbarController.setQuery(category, true);
  }

  private void refreshSearchResults(@NonNull SearchResult[] results)
  {
    mSearchRunning = true;
    hideShimmer();
    updateFrames();
    mSearchAdapter.refreshData(results);
    mSearchViewModel.setLastResults(results);
    mSearchViewModel.setSearchQuery(getQuery());
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
      return true;
    }

    return mSearchFragmentListener.getBackPressedCallback();
  }

  public void setRecyclerScrollListener(RecyclerView recycler)
  {
    recycler.addOnScrollListener(mRecyclerListener);
    mAttachedRecyclers.add(recycler);
    if (mTabAdapter != null && mPager != null)
      updateNestedScrollingForTab(mTabAdapter, mPager.getCurrentItem());
  }

  private void updateNestedScrollingForTab(@NonNull TabAdapter tabAdapter, int selectedPosition)
  {
    for (int i = 0; i < tabAdapter.getCount(); i++)
    {
      Fragment fragment = tabAdapter.getItem(i);
      if (fragment == null || fragment.getView() == null)
        continue;

      RecyclerView rv = fragment.getView().findViewById(R.id.recycler);
      if (rv != null)
        ViewCompat.setNestedScrollingEnabled(rv, i == selectedPosition);
    }

    View bottomSheet = getBottomSheetContainer();
    if (bottomSheet != null)
      bottomSheet.requestLayout();
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
    if (mToolbarController != null)
      mToolbarController.activate();
  }

  interface SearchFragmentListener
  {
    boolean getBackPressedCallback();
    void onSearchClicked();
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
    }

    @Override
    public void setQuery(CharSequence query)
    {
      super.setQuery(query);
      if (query != "")
        mSearchFragmentListener.onSearchClicked();
    }

    @Override
    protected void onTextChanged(String query)
    {
      if (!isAdded())
        return;

      if (TextUtils.isEmpty(query))
      {
        mSearchAdapter.clear();
        mSearchViewModel.setLastResults(null);
        stopSearch();
        return;
      }

      if (tryRecognizeHiddenCommand(query))
      {
        mSearchAdapter.clear();
        stopSearch();
        requireActivity().finish();
        return;
      }

      runSearch();
    }

    @Override
    protected boolean onStartSearchClick()
    {
      if (Config.isSearchHistoryEnabled())
      {
        SearchRecents.add(getQuery(), requireContext());
        refreshHistoryFragment();
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
      if (!onBackPressed())
        super.onUpClick();
    }

    @Override
    public void clear()
    {
      super.clear();
    }
  }
}
