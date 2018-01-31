package com.mapswithme.maps.search;

import android.app.Activity;
import android.content.Intent;
import android.location.Location;
import android.os.Build;
import android.os.Bundle;
import android.support.annotation.CallSuper;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.design.widget.AppBarLayout;
import android.support.design.widget.CollapsingToolbarLayout;
import android.support.design.widget.TabLayout;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentManager;
import android.support.v4.view.ViewPager;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.google.android.gms.ads.search.SearchAdView;
import com.mapswithme.maps.MwmActivity;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmFragment;
import com.mapswithme.maps.base.OnBackPressListener;
import com.mapswithme.maps.bookmarks.data.FeatureId;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.downloader.CountrySuggestFragment;
import com.mapswithme.maps.downloader.MapManager;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.location.LocationListener;
import com.mapswithme.maps.routing.RoutingController;
import com.mapswithme.maps.widget.PlaceholderView;
import com.mapswithme.maps.widget.SearchToolbarController;
import com.mapswithme.maps.widget.placepage.Sponsored;
import com.mapswithme.util.ConnectionState;
import com.mapswithme.util.SharedPropertiesUtils;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.concurrency.UiThread;
import com.mapswithme.util.log.LoggerFactory;
import com.mapswithme.util.statistics.Statistics;

import java.util.ArrayList;
import java.util.LinkedList;
import java.util.List;

public class SearchFragment extends BaseMwmFragment
                         implements OnBackPressListener,
                                    NativeSearchListener,
                                    SearchToolbarController.Container,
                                    CategoriesAdapter.OnCategorySelectedListener,
                                    HotelsFilterHolder
{
  public static final String PREFS_SHOW_ENABLE_LOGGING_SETTING = "ShowEnableLoggingSetting";
  private static final int MIN_QUERY_LENGTH_FOR_AD = 3;
  private static final long ADS_DELAY_MS = 200;
  private static final long RESULTS_DELAY_MS = 400;
  private static final int AD_POSITION = 3;

  private long mLastQueryTimestamp;

  @Nullable
  private GoogleAdsLoader mAdsLoader;
  private boolean mAdsRequested = false;
  private int mAdsOrientation = -1;
  @Nullable
  private SearchAdView mGoogleAdView;
  @NonNull
  private final Runnable mResultsShowingTask = this::refreshSearchResults;
  @NonNull
  private final Runnable mSearchEndTask = this::onSearchEnd;

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
      super(root, getActivity());
    }

    @Override
    protected boolean useExtendedToolbar()
    {
      return false;
    }

    @Override
    protected void onTextChanged(String query)
    {
      if (!isAdded())
        return;

      UiThread.cancelDelayedTasks(mSearchEndTask);
      UiThread.cancelDelayedTasks(mResultsShowingTask);
      mGoogleAdView = null;
      stopAdsLoading();

      if (TextUtils.isEmpty(query))
      {
        mSearchAdapter.clear();
        stopSearch();
        return;
      }

      if (tryRecognizeLoggingCommand(query))
      {
        mSearchAdapter.clear();
        stopSearch();
        closeSearch();
        return;
      }

      if (mLuggageCategorySelected)
        return;

      if (mAdsLoader != null && !isInteractiveSearch() && query.length() >= MIN_QUERY_LENGTH_FOR_AD)
      {
        mAdsRequested = true;
        mAdsLoader.scheduleAdsLoading(getActivity(), query);
      }

      runSearch();
    }

    @Override
    protected boolean onStartSearchClick()
    {
      if (!RoutingController.get().isWaitingPoiPick())
        showAllResultsOnMap();
      return true;
    }

    @Override
    protected int getVoiceInputPrompt()
    {
      return R.string.search_map;
    }

    @Override
    protected void startVoiceRecognition(Intent intent, int code)
    {
      startActivityForResult(intent, code);
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
      if (mFilterController != null)
        mFilterController.resetFilter();
    }
  }

  private View mTabFrame;
  private View mResultsFrame;
  private PlaceholderView mResultsPlaceholder;
  private RecyclerView mResults;
  private AppBarLayout mAppBarLayout;
  private CollapsingToolbarLayout mToolbarLayout;
  private View mFilterElevation;
  @Nullable
  private SearchFilterController mFilterController;

  private SearchToolbarController mToolbarController;

  private SearchAdapter mSearchAdapter;

  private final List<RecyclerView> mAttachedRecyclers = new ArrayList<>();
  private final RecyclerView.OnScrollListener mRecyclerListener = new RecyclerView.OnScrollListener()
  {
    @Override
    public void onScrollStateChanged(RecyclerView recyclerView, int newState)
    {
      if (newState == RecyclerView.SCROLL_STATE_DRAGGING)
        mToolbarController.deactivate();
    }
  };

  private final LastPosition mLastPosition = new LastPosition();
  private boolean mSearchRunning;
  private boolean mLuggageCategorySelected;
  private String mInitialQuery;
  @Nullable
  private String mInitialLocale;
  private boolean mInitialSearchOnMap = false;
  @Nullable
  private HotelsFilter mInitialHotelsFilter;
  @Nullable
  private BookingFilterParams mInitialFilterParams;

  private boolean mIsHotel;
  @NonNull
  private SearchResult[] mSearchResults = new SearchResult[0];

  private final LocationListener mLocationListener = new LocationListener.Simple()
  {
    @Override
    public void onLocationUpdated(Location location)
    {
      mLastPosition.set(location.getLatitude(), location.getLongitude());

      if (!TextUtils.isEmpty(getQuery()))
        mSearchAdapter.notifyDataSetChanged();
    }
  };

  private final AppBarLayout.OnOffsetChangedListener mOffsetListener =
      new AppBarLayout.OnOffsetChangedListener() {
        @Override
        public void onOffsetChanged(AppBarLayout appBarLayout, int verticalOffset)
        {
          if (mFilterController == null)
            return;

          boolean show = !(Math.abs(verticalOffset) == appBarLayout.getTotalScrollRange());
          mFilterController.showDivider(show);
          if (Build.VERSION.SDK_INT < Build.VERSION_CODES.LOLLIPOP)
            UiUtils.showIf(!show, mFilterElevation);
        }
      };

  private static boolean doShowDownloadSuggest()
  {
    return (MapManager.nativeGetDownloadedCount() == 0 && !MapManager.nativeIsDownloading());
  }

  @Override
  @Nullable
  public HotelsFilter getHotelsFilter()
  {
    if (mFilterController == null)
      return null;

    return mFilterController.getFilter();
  }

  @Nullable
  @Override
  public BookingFilterParams getFilterParams()
  {
    if (mFilterController == null)
      return null;

    return mFilterController.getBookingFilterParams();
  }

  private void showDownloadSuggest()
  {
    final FragmentManager fm = getChildFragmentManager();
    final String fragmentName = CountrySuggestFragment.class.getName();
    Fragment fragment = fm.findFragmentByTag(fragmentName);

    if (fragment == null || fragment.isDetached() || fragment.isRemoving())
    {
      fragment = Fragment.instantiate(getActivity(), fragmentName, null);
      fm.beginTransaction()
        .add(R.id.download_suggest_frame, fragment, fragmentName)
        .commit();
    }
  }

  private void hideDownloadSuggest()
  {
    final FragmentManager manager = getChildFragmentManager();
    final Fragment fragment = manager.findFragmentByTag(CountrySuggestFragment.class.getName());
    if (fragment != null && !fragment.isDetached() && !fragment.isRemoving())
      manager.beginTransaction()
             .remove(fragment)
             .commitAllowingStateLoss();
  }

  private void updateFrames()
  {
    final boolean hasQuery = mToolbarController.hasQuery();
    UiUtils.showIf(hasQuery, mResultsFrame);
    AppBarLayout.LayoutParams lp = (AppBarLayout.LayoutParams) mToolbarLayout.getLayoutParams();
    lp.setScrollFlags(hasQuery ? AppBarLayout.LayoutParams.SCROLL_FLAG_ENTER_ALWAYS
                        | AppBarLayout.LayoutParams.SCROLL_FLAG_SCROLL : 0);
    mToolbarLayout.setLayoutParams(lp);
    if (mFilterController != null)
      mFilterController.show(hasQuery && mSearchAdapter.getItemCount() != 0,
                             mSearchAdapter.showPopulateButton());

    if (hasQuery)
      hideDownloadSuggest();
    else if (doShowDownloadSuggest())
      showDownloadSuggest();
    else
      hideDownloadSuggest();
  }

  private void updateResultsPlaceholder()
  {
    final boolean show = !mSearchRunning
                         && mSearchAdapter.getItemCount() == 0
                         && mToolbarController.hasQuery();

    UiUtils.showIf(show, mResultsPlaceholder);
    if (mFilterController != null)
      mFilterController.showPopulateButton(mSearchAdapter.showPopulateButton());
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
    readArguments();

    if (ConnectionState.isWifiConnected() && SharedPropertiesUtils.isShowcaseSwitchedOnLocal())
    {
      mAdsLoader = new GoogleAdsLoader(getContext(), ADS_DELAY_MS);
      mAdsLoader.attach(searchAdView ->
                        {
                          mGoogleAdView = searchAdView;
                          mAdsRequested = false;
                        });
    }

    ViewGroup root = (ViewGroup) view;
    mAppBarLayout = root.findViewById(R.id.app_bar);
    mToolbarLayout = mAppBarLayout.findViewById(R.id.collapsing_toolbar);
    mTabFrame = root.findViewById(R.id.tab_frame);
    ViewPager pager = mTabFrame.findViewById(R.id.pages);

    mToolbarController = new ToolbarController(view);

    TabLayout tabLayout = root.findViewById(R.id.tabs);
    final TabAdapter tabAdapter = new TabAdapter(getChildFragmentManager(), pager, tabLayout);

    mResultsFrame = root.findViewById(R.id.results_frame);
    mResults = mResultsFrame.findViewById(R.id.recycler);
    setRecyclerScrollListener(mResults);
    mResultsPlaceholder = mResultsFrame.findViewById(R.id.placeholder);
    mResultsPlaceholder.setContent(R.drawable.img_mappyny,
                                   R.string.search_not_found, R.string.search_not_found_query);

    mFilterElevation = view.findViewById(R.id.filter_elevation);

    mFilterController = new SearchFilterController(root.findViewById(R.id.filter_frame),
                                                   new SearchFilterController.DefaultFilterListener()
    {
      @Override
      public void onShowOnMapClick()
      {
        showAllResultsOnMap();
      }

      @Override
      public void onFilterClick()
      {
        HotelsFilter filter = null;
        BookingFilterParams params = null;
        if (mFilterController != null)
        {
          filter = mFilterController.getFilter();
          params = mFilterController.getBookingFilterParams();
        }
        FilterActivity.startForResult(SearchFragment.this, filter, params,
                                      FilterActivity.REQ_CODE_FILTER);
      }

      @Override
      public void onFilterClear()
      {
        runSearch();
      }
    });
    if (savedInstanceState != null)
      mFilterController.onRestoreState(savedInstanceState);
    if (mInitialHotelsFilter != null)
      mFilterController.setFilter(mInitialHotelsFilter);
    mFilterController.setBookingFilterParams(mInitialFilterParams);
    mFilterController.updateFilterButtonVisibility(false);

    if (mSearchAdapter == null)
    {
      mSearchAdapter = new SearchAdapter(this);
      mSearchAdapter.registerAdapterDataObserver(new RecyclerView.AdapterDataObserver()
      {
        @Override
        public void onChanged()
        {
          updateResultsPlaceholder();
        }
      });
    }

    mResults.setLayoutManager(new LinearLayoutManager(view.getContext()));
    mResults.setAdapter(mSearchAdapter);

    updateFrames();
    updateResultsPlaceholder();

    if (mInitialQuery != null)
    {
      setQuery(mInitialQuery);
    }
    mToolbarController.activate();

    SearchEngine.INSTANCE.addListener(this);

    if (SearchRecents.getSize() == 0)
      pager.setCurrentItem(TabAdapter.Tab.CATEGORIES.ordinal());

    tabAdapter.setTabSelectedListener(tab ->
                                      {
                                        Statistics.INSTANCE.trackSearchTabSelected(tab.name());
                                        mToolbarController.deactivate();
                                      });

    if (mInitialSearchOnMap)
      showAllResultsOnMap();
  }

  @Override
  public void onSaveInstanceState(Bundle outState)
  {
    if (mFilterController != null)
      mFilterController.onSaveState(outState);
  }

  public void onResume()
  {
    super.onResume();
    LocationHelper.INSTANCE.addListener(mLocationListener, true);
    mAppBarLayout.addOnOffsetChangedListener(mOffsetListener);
  }

  @Override
  public void onPause()
  {
    LocationHelper.INSTANCE.removeListener(mLocationListener);
    super.onPause();
    mAppBarLayout.removeOnOffsetChangedListener(mOffsetListener);
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

  @Override
  public void onDestroyView()
  {
    if (mAdsLoader != null)
      mAdsLoader.detach();
    super.onDestroyView();
  }

  private String getQuery()
  {
    return mToolbarController.getQuery();
  }

  void setQuery(String text)
  {
    mToolbarController.setQuery(text);
  }

  private void readArguments()
  {
    final Bundle arguments = getArguments();
    if (arguments == null)
      return;

    mInitialQuery = arguments.getString(SearchActivity.EXTRA_QUERY);
    mInitialLocale = arguments.getString(SearchActivity.EXTRA_LOCALE);
    mInitialSearchOnMap = arguments.getBoolean(SearchActivity.EXTRA_SEARCH_ON_MAP);
    mInitialHotelsFilter = arguments.getParcelable(FilterActivity.EXTRA_FILTER);
    mInitialFilterParams = arguments.getParcelable(FilterActivity.EXTRA_FILTER_PARAMS);
  }

  private boolean tryRecognizeLoggingCommand(@NonNull String str)
  {
    if (str.equals("?enableLogging"))
    {
      MwmApplication.prefs().edit().putBoolean(PREFS_SHOW_ENABLE_LOGGING_SETTING, true).apply();
      return true;
    }

    if (str.equals("?disableLogging"))
    {
      LoggerFactory.INSTANCE.setFileLoggingEnabled(false);
      MwmApplication.prefs().edit().putBoolean(PREFS_SHOW_ENABLE_LOGGING_SETTING, false).apply();
      return true;
    }

    if (str.equals("?emulateBadStorage"))
    {
      SharedPropertiesUtils.setShouldShowEmulateBadStorageSetting(true);
      return true;
    }

    return false;
  }

  private void processSelected(@NonNull SearchResult result)
  {
    if (RoutingController.get().isWaitingPoiPick())
    {
      SearchResult.Description description = result.description;
      final MapObject point = MapObject.createMapObject(FeatureId.EMPTY, MapObject.SEARCH, result.name,
                                            description != null ? description.featureType : "",
                                            result.lat, result.lon);
      RoutingController.get().onPoiSelected(point);
    }

    if (mFilterController != null)
      mFilterController.resetFilter();

    mToolbarController.deactivate();

    if (getActivity() instanceof SearchActivity)
      Utils.navigateToParent(getActivity());
  }

  void showSingleResultOnMap(@NonNull SearchResult result, int resultIndex)
  {
    final String query = getQuery();
    SearchRecents.add(query);
    SearchEngine.cancelApiCall();

    if (!RoutingController.get().isWaitingPoiPick())
      SearchEngine.showResult(resultIndex);

    processSelected(result);

    Statistics.INSTANCE.trackEvent(Statistics.EventName.SEARCH_ITEM_CLICKED);
  }

  void showAllResultsOnMap()
  {
    final String query = getQuery();
    SearchRecents.add(query);
    mLastQueryTimestamp = System.nanoTime();

    HotelsFilter hotelsFilter = null;
    BookingFilterParams bookingFilterParams = null;
    if (mFilterController != null)
    {
      hotelsFilter = mFilterController.getFilter();
      bookingFilterParams = mFilterController.getBookingFilterParams();
    }

    SearchEngine.searchInteractive(
        query, !TextUtils.isEmpty(mInitialLocale)
               ? mInitialLocale : com.mapswithme.util.Language.getKeyboardLocale(),
        mLastQueryTimestamp, false /* isMapAndTable */,
        hotelsFilter, bookingFilterParams);
    SearchEngine.setQuery(query);
    Utils.navigateToParent(getActivity());

    Statistics.INSTANCE.trackEvent(Statistics.EventName.SEARCH_ON_MAP_CLICKED);
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
    SearchEngine.cancelApiCall();
    SearchEngine.cancelAllSearches();
    updateSearchView();
  }

  private boolean isInteractiveSearch()
  {
    // TODO @yunitsky Implement more elegant solution.
    return getActivity() instanceof MwmActivity;
  }

  private void runSearch()
  {
    HotelsFilter hotelsFilter = null;
    BookingFilterParams bookingFilterParams = null;
    if (mFilterController != null)
    {
      hotelsFilter = mFilterController.getFilter();
      bookingFilterParams = mFilterController.getBookingFilterParams();
    }

    mLastQueryTimestamp = System.nanoTime();
    if (isInteractiveSearch())
    {
      SearchEngine.searchInteractive(getQuery(), mLastQueryTimestamp, true /* isMapAndTable */,
                                     hotelsFilter, bookingFilterParams);
    }
    else
    {
      if (!SearchEngine.search(getQuery(), mLastQueryTimestamp, mLastPosition.valid,
                               mLastPosition.lat, mLastPosition.lon,
                               hotelsFilter, bookingFilterParams))
      {
        return;
      }
    }

    mSearchRunning = true;
    mToolbarController.showProgress(true);

    updateFrames();
  }

  @Override
  public void onResultsUpdate(SearchResult[] results, long timestamp, boolean isHotel)
  {
    if (!isAdded() || !mToolbarController.hasQuery())
      return;

    mSearchResults = results;
    mIsHotel = isHotel;

    if (mAdsRequested)
    {
      UiThread.cancelDelayedTasks(mResultsShowingTask);
      UiThread.runLater(mResultsShowingTask, RESULTS_DELAY_MS);
    }
    else
    {
      refreshSearchResults();
    }
  }

  @Override
  public void onResultsEnd(long timestamp)
  {
    if (mAdsRequested)
    {
      UiThread.cancelDelayedTasks(mSearchEndTask);
      UiThread.runLater(mSearchEndTask, RESULTS_DELAY_MS);
    }
    else
    {
      onSearchEnd();
    }
  }

  @Override
  public void onCategorySelected(String category)
  {
    if (!TextUtils.isEmpty(category) &&
        category.equals(CategoriesAdapter.LUGGAGE_CATEGORY + ' '))
    {
      mLuggageCategorySelected = true;
      mToolbarController.setQuery(category);

      Statistics.INSTANCE.trackSponsoredEventForCustomProvider(
          Statistics.EventName.SEARCH_SPONSOR_CATEGORY_SELECTED,
          Statistics.ParamValue.LUGGAGE_HERO);
      showAllResultsOnMap();
      mLuggageCategorySelected = false;
    }
    else
    {
      mToolbarController.setQuery(category);
    }
  }

  private void refreshSearchResults()
  {
    // Search is running hence results updated.
    stopAdsLoading();
    mSearchRunning = true;
    updateFrames();
    mSearchAdapter.refreshData(combineResultsWithAds());
    mToolbarController.showProgress(true);
    updateFilterButton(mIsHotel);
  }

  @NonNull
  private SearchData[] combineResultsWithAds()
  {
    if (mSearchResults.length < AD_POSITION || mGoogleAdView == null)
      return mSearchResults;

    List<SearchData> result = new LinkedList<>();
    int counter = 0;
    for (SearchResult r : mSearchResults)
    {
      if (r.type != SearchResultTypes.TYPE_SUGGEST && counter++ == AD_POSITION)
        result.add(new GoogleAdsBanner(mGoogleAdView));
      else
        result.add(r);
    }

    SearchData[] resultArray = new SearchData[result.size()];
    result.toArray(resultArray);
    return resultArray;
  }

  private void updateFilterButton(boolean isHotel)
  {
    if (mFilterController != null)
    {
      mFilterController.updateFilterButtonVisibility(isHotel);
      if (!isHotel)
        mFilterController.setFilter(null);
    }
  }

  @Override
  public void onActivityResult(int requestCode, int resultCode, Intent data)
  {
    super.onActivityResult(requestCode, resultCode, data);
    mToolbarController.onActivityResult(requestCode, resultCode, data);

    if (resultCode != Activity.RESULT_OK)
      return;

    switch (requestCode)
    {
      case FilterActivity.REQ_CODE_FILTER:
        if (data == null)
          break;

        if (mFilterController == null)
          return;

        mFilterController.setFilter(data.getParcelableExtra(FilterActivity.EXTRA_FILTER));
        BookingFilterParams params = data.getParcelableExtra(FilterActivity.EXTRA_FILTER_PARAMS);
        mFilterController.setBookingFilterParams(params);
        runSearch();
        break;
    }
  }

  @Override
  public boolean onBackPressed()
  {
    if (mToolbarController.hasQuery())
    {
      mToolbarController.clear();
      return true;
    }

    boolean isSearchActivity = getActivity() instanceof SearchActivity;
    mToolbarController.deactivate();
    if (RoutingController.get().isWaitingPoiPick())
    {
      RoutingController.get().onPoiSelected(null);
      if (isSearchActivity)
        closeSearch();
      return !isSearchActivity;
    }

    if (isSearchActivity)
      closeSearch();
    return isSearchActivity;
  }

  private void closeSearch()
  {
    getActivity().finish();
    getActivity().overridePendingTransition(android.R.anim.fade_in, android.R.anim.fade_out);
  }

  public void setRecyclerScrollListener(RecyclerView recycler)
  {
    recycler.addOnScrollListener(mRecyclerListener);
    mAttachedRecyclers.add(recycler);
  }

  @Override
  public SearchToolbarController getController()
  {
    return mToolbarController;
  }

  private void stopAdsLoading()
  {
    if (mAdsLoader == null)
      return;

    mAdsLoader.cancelAdsLoading();
    mAdsRequested = false;
  }
}
