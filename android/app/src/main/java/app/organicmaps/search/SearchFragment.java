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
import androidx.appcompat.widget.Toolbar;
import androidx.core.view.ViewCompat;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentManager;
import androidx.lifecycle.Observer;
import androidx.lifecycle.ViewModelProvider;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import androidx.viewpager.widget.ViewPager;
import app.organicmaps.MwmActivity;
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
import app.organicmaps.util.Utils;
import app.organicmaps.widget.PlaceholderView;
import app.organicmaps.widget.SearchToolbarController;
import com.google.android.material.appbar.AppBarLayout;
import com.google.android.material.bottomsheet.BottomSheetBehavior;
import com.google.android.material.tabs.TabLayout;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class SearchFragment extends Fragment implements SearchListener, CategoriesAdapter.CategoriesUiListener
{
  private long mLastQueryTimestamp;
  @NonNull
  private final List<HiddenCommand> mHiddenCommands = new ArrayList<>();
  private SearchFragmentListener mSearchFragmentListener;

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

  private View mResultsFrame;
  private PlaceholderView mResultsPlaceholder;
  private SearchPageViewModel mSearchViewModel;

  @NonNull
  private SearchToolbarController mToolbarController;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private SearchAdapter mSearchAdapter;

  private final List<RecyclerView> mAttachedRecyclers = new ArrayList<>();
  private final RecyclerView.OnScrollListener mRecyclerListener = new RecyclerView.OnScrollListener() {
    @Override
    public void onScrollStateChanged(@NonNull RecyclerView recyclerView, int newState)
    {
      if (newState == RecyclerView.SCROLL_STATE_DRAGGING)
        mToolbarController.deactivate();
    }
  };

  private final LastPosition mLastPosition = new LastPosition();
  private boolean mSearchRunning;
  private String mInitialQuery;
  @Nullable
  private String mInitialLocale;

  private final ActivityResultLauncher<Intent> startVoiceRecognitionForResult =
      registerForActivityResult(new ActivityResultContracts.StartActivityForResult(),
                                activityResult -> { mToolbarController.onVoiceRecognitionResult(activityResult); });

  private final LocationListener mLocationListener = new LocationListener() {
    @Override
    public void onLocationUpdated(@NonNull Location location)
    {
      mLastPosition.set(location.getLatitude(), location.getLongitude());

      if (!TextUtils.isEmpty(getQuery()))
        mSearchAdapter.notifyDataSetChanged();
    }
  };

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
      if (enabled != null && !enabled)
      {
        if (mToolbarController.hasQuery())
          mToolbarController.clear();
        mSearchViewModel.setSearchQuery(null);
        mSearchViewModel.setLastResults(null);
      }
    }
  };

  private final Observer<Integer> mBottomSheetStateObserver = new Observer<>() {
    public void onChanged(Integer state)
    {
      if (state != BottomSheetBehavior.STATE_EXPANDED)
        mToolbarController.deactivate();
    }
  };

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
    Toolbar toolbar = mToolbarController.getToolbar();
    AppBarLayout.LayoutParams lp = (AppBarLayout.LayoutParams) toolbar.getLayoutParams();
    lp.setScrollFlags(0); // keep toolbar always visible
    toolbar.setLayoutParams(lp);

    UiUtils.showIf(hasQuery, mResultsFrame);
    if (hasQuery)
      hideDownloadSuggest();
    else if (doShowDownloadSuggest())
      showDownloadSuggest();
    else
      hideDownloadSuggest();
  }

  private void updateResultsPlaceholder()
  {
    final boolean show = !mSearchRunning && mSearchAdapter.getItemCount() == 0 && mToolbarController.hasQuery();

    UiUtils.showIf(show, mResultsPlaceholder);
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
    View mTabFrame = root.findViewById(R.id.tab_frame);
    ViewPager pager = mTabFrame.findViewById(R.id.pages);

    mToolbarController = new ToolbarController(view);
    if (savedInstanceState != null)
      mToolbarController.skipNextTextChange();
    TabLayout tabLayout = root.findViewById(R.id.tabs);
    mResultsFrame = root.findViewById(R.id.results_frame);
    RecyclerView mResults = mResultsFrame.findViewById(R.id.recycler);
    setRecyclerScrollListener(mResults);
    mResultsPlaceholder = mResultsFrame.findViewById(R.id.placeholder);
    mResultsPlaceholder.setContent(R.string.search_not_found, R.string.search_not_found_query);
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

    mSearchViewModel.getSearchPageLastState().observe(requireActivity(), mBottomSheetStateObserver);

    if (Config.isSearchHistoryEnabled())
      tabLayout.setVisibility(View.VISIBLE);
    else
      tabLayout.setVisibility(View.GONE);

    // Measure and report toolbar height for collapsed state peek height
    // Include handle, toolbar, and tab layout (categories/history tabs)
    View pullIconContainer = root.findViewById(R.id.pull_icon_container);
    Toolbar toolbar = mToolbarController.getToolbar();
    TabLayout finalTabLayout = tabLayout;
    toolbar.post(() -> {
      int toolbarHeight = toolbar.getHeight();
      int handleHeight = pullIconContainer != null ? pullIconContainer.getHeight() : 0;
      int tabLayoutHeight = finalTabLayout.getVisibility() == View.VISIBLE ? finalTabLayout.getHeight() : 0;
      mSearchViewModel.setToolbarHeight(handleHeight + toolbarHeight + tabLayoutHeight);
    });

    final TabAdapter tabAdapter = new TabAdapter(getChildFragmentManager(), pager, tabLayout);

    //    readArguments();
    updateFrames();
    updateResultsPlaceholder();

    mToolbarController.activate();

    SearchEngine.INSTANCE.addListener(this);

    SharedPreferences preferences = MwmApplication.prefs(requireContext());
    int lastSelectedTabPosition = preferences.getInt(Config.KEY_PREF_LAST_SEARCHED_TAB, 0);
    if (SearchRecents.getSize() == 0 && Config.isSearchHistoryEnabled())
      pager.setCurrentItem(lastSelectedTabPosition);

    tabAdapter.setTabSelectedListener(tab -> mToolbarController.deactivate());
  }

  @Override
  public void onStart()
  {
    Logger.d("kavi", "on start got called for search fragment");
    super.onStart();
    mToolbarController.attach(requireActivity());
    mSearchViewModel.getSearchEnabled().observe(getViewLifecycleOwner(), mSearchEnabledObserver);
  }

  public void onResume()
  {
    Logger.d("kavi", "on resume got called for search fragment");
    super.onResume();
    MwmApplication.from(requireContext()).getLocationHelper().addListener(mLocationListener);
    if (mInitialQuery != null && !mInitialQuery.isEmpty())
    {
      setQuery(mInitialQuery, false);
      mInitialQuery = null;
    }
  }

  @Override
  public void onPause()
  {
    Logger.d("kavi", "on pause got called for search fragment");
    MwmApplication.from(requireContext()).getLocationHelper().removeListener(mLocationListener);
    super.onPause();
  }

  @Override
  public void onStop()
  {
    Logger.d("kavi", "on stop got called for search fragment");
    super.onStop();
    mSearchViewModel.setLastResults(mSearchAdapter.getResults());
    mSearchViewModel.setSearchQuery(TextUtils.isEmpty(getQuery()) ? null : getQuery());
    mToolbarController.detach();
    mSearchViewModel.getSearchPageLastState().removeObserver(mBottomSheetStateObserver);
    mSearchViewModel.getSearchEnabled().observe(getViewLifecycleOwner(), mSearchEnabledObserver);
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

  private void readArguments()
  {
    if (mSearchViewModel.getSearchQuery() != null && !mSearchViewModel.getSearchQuery().isEmpty())
    {
      mInitialQuery = mSearchViewModel.getSearchQuery();
      mSearchViewModel.setSearchQuery(null);
    }
    //    mInitialLocale = arguments.getString(SearchActivity.EXTRA_LOCALE);
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
      SearchRecents.add(query, requireContext());
    SearchEngine.INSTANCE.cancel();
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
      SearchEngine.INSTANCE.showResult(resultIndex);
    }

    mToolbarController.deactivate();
  }

  void showAllResultsOnMap()
  {
    SearchEngine.INSTANCE.updateViewportWithLastResults();

    // The previous search should be cancelled before the new one is started, since previous search
    // results are no longer needed.
    SearchEngine.INSTANCE.cancel();

    final String query = getQuery();
    if (Config.isSearchHistoryEnabled())
      SearchRecents.add(query, requireContext());
    mLastQueryTimestamp = System.nanoTime();

    SearchEngine.INSTANCE.searchInteractive(
        query, isCategory(),
        !TextUtils.isEmpty(mInitialLocale) ? mInitialLocale : Language.getKeyboardLocale(requireContext()),
        mLastQueryTimestamp, false /* isMapAndTable */);

    SearchEngine.INSTANCE.setQuery(query);
    Utils.navigateToParent(requireActivity());
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
    updateFrames();
    updateResultsPlaceholder();
  }

  private void stopSearch()
  {
    SearchEngine.INSTANCE.cancel();
    updateSearchView();
  }

  private boolean isTabletSearch()
  {
    // TODO @yunitsky Implement more elegant solution.
    return requireActivity() instanceof MwmActivity;
  }

  private void runSearch()
  {
    // The previous search should be cancelled before the new one is started, since previous search
    // results are no longer needed.
    SearchEngine.INSTANCE.cancel();

    mLastQueryTimestamp = System.nanoTime();
    if (isTabletSearch())
    {
      SearchEngine.INSTANCE.searchInteractive(requireContext(), getQuery(), isCategory(), mLastQueryTimestamp,
                                              true /* isMapAndTable */);
    }
    else
    {
      if (!SearchEngine.INSTANCE.search(requireContext(), getQuery(), isCategory(), mLastQueryTimestamp,
                                        mLastPosition.valid, mLastPosition.lat, mLastPosition.lon))
      {
        return;
      }
    }

    mSearchRunning = true;
    mToolbarController.showProgress(true);

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
    mToolbarController.setQuery(category, true);
  }

  private void refreshSearchResults(@NonNull SearchResult[] results)
  {
    mSearchRunning = true;
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

    if (mSearchFragmentListener.getBackPressedCallback())
      return true;
    return false;
  }

  public void setRecyclerScrollListener(RecyclerView recycler)
  {
    recycler.addOnScrollListener(mRecyclerListener);
    mAttachedRecyclers.add(recycler);
  }

  @NonNull
  public SearchToolbarController requireController()
  {
    return mToolbarController;
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

  interface SearchFragmentListener
  {
    boolean getBackPressedCallback();
    void onSearchClicked();
  }
  
  public void activateToolbar()
  {
    if (mToolbarController != null)
      mToolbarController.activate();
  }
}
