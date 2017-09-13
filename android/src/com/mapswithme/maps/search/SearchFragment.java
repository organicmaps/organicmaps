package com.mapswithme.maps.search;

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
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.log.LoggerFactory;
import com.mapswithme.util.statistics.Statistics;

import java.util.ArrayList;
import java.util.List;

public class SearchFragment extends BaseMwmFragment
                         implements OnBackPressListener,
                                    NativeSearchListener,
                                    SearchToolbarController.Container,
                                    CategoriesAdapter.OnCategorySelectedListener,
                                    HotelsFilterHolder
{
  public static final String PREFS_SHOW_ENABLE_LOGGING_SETTING = "ShowEnableLoggingSetting";

  private long mLastQueryTimestamp;

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

      if (mCianCategorySelected)
        return;

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
      {
        mFilterController.setFilter(null);
        mFilterController.updateFilterButtonVisibility(false);
      }
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
  private boolean mCianCategorySelected;
  private String mInitialQuery;
  @Nullable
  private String mInitialLocale;
  private boolean mInitialSearchOnMap = false;
  @Nullable
  private HotelsFilter mInitialHotelsFilter;

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

    ViewGroup root = (ViewGroup) view;
    mAppBarLayout = (AppBarLayout) root.findViewById(R.id.app_bar);
    mToolbarLayout = (CollapsingToolbarLayout) mAppBarLayout.findViewById(R.id.collapsing_toolbar);
    mTabFrame = root.findViewById(R.id.tab_frame);
    ViewPager pager = (ViewPager) mTabFrame.findViewById(R.id.pages);

    mToolbarController = new ToolbarController(view);

    TabLayout tabLayout = (TabLayout) root.findViewById(R.id.tabs);
    final TabAdapter tabAdapter = new TabAdapter(getChildFragmentManager(), pager, tabLayout);

    mResultsFrame = root.findViewById(R.id.results_frame);
    mResults = (RecyclerView) mResultsFrame.findViewById(R.id.recycler);
    setRecyclerScrollListener(mResults);
    mResultsPlaceholder = (PlaceholderView) mResultsFrame.findViewById(R.id.placeholder);
    mResultsPlaceholder.setContent(R.drawable.img_search_nothing_found_light,
                                   R.string.search_not_found, R.string.search_not_found_query);

    mFilterElevation = view.findViewById(R.id.filter_elevation);

    mFilterController = new SearchFilterController(root.findViewById(R.id.filter_frame),
                                                   (HotelsFilterView) view.findViewById(R.id.filter),
                                                   new SearchFilterController.DefaultFilterListener()
    {
      @Override
      public void onViewClick()
      {
        showAllResultsOnMap();
      }

      @Override
      public void onFilterClear()
      {
        runSearch();
      }

      @Override
      public void onFilterDone()
      {
        runSearch();
      }
    });
    if (savedInstanceState != null)
      mFilterController.onRestoreState(savedInstanceState);
    if (mInitialHotelsFilter != null)
      mFilterController.setFilter(mInitialHotelsFilter);
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

    tabAdapter.setTabSelectedListener(new TabAdapter.OnTabSelectedListener()
    {
      @Override
      public void onTabSelected(@NonNull TabAdapter.Tab tab)
      {
        Statistics.INSTANCE.trackSearchTabSelected(tab.name());
        mToolbarController.deactivate();
      }
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
    mInitialHotelsFilter = arguments.getParcelable(SearchActivity.EXTRA_HOTELS_FILTER);
  }

  private void hideSearch()
  {
    mToolbarController.clear();
    mToolbarController.deactivate();
    Utils.navigateToParent(getActivity());
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
    if (mFilterController != null)
      hotelsFilter = mFilterController.getFilter();

    SearchEngine.searchInteractive(
        query, !TextUtils.isEmpty(mInitialLocale)
               ? mInitialLocale : com.mapswithme.util.Language.getKeyboardLocale(),
        mLastQueryTimestamp, false /* isMapAndTable */, hotelsFilter);
    SearchEngine.showAllResults(query);
    Utils.navigateToParent(getActivity());

    Statistics.INSTANCE.trackEvent(Statistics.EventName.SEARCH_ON_MAP_CLICKED);
  }

  private void onSearchEnd()
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
    onSearchEnd();
  }

  private void runSearch()
  {
    HotelsFilter hotelsFilter = null;
    if (mFilterController != null)
      hotelsFilter = mFilterController.getFilter();

    mLastQueryTimestamp = System.nanoTime();
    // TODO @yunitsky Implement more elegant solution.
    if (getActivity() instanceof MwmActivity)
    {
      SearchEngine.searchInteractive(
          getQuery(), mLastQueryTimestamp, true /* isMapAndTable */, hotelsFilter);
    }
    else
    {
      if (!SearchEngine.search(getQuery(), mLastQueryTimestamp, mLastPosition.valid,
              mLastPosition.lat, mLastPosition.lon, hotelsFilter))
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

    // Search is running hence results updated.
    mSearchRunning = true;
    updateFrames();
    mSearchAdapter.refreshData(results);
    mToolbarController.showProgress(true);
    updateFilterButton(isHotel);
  }

  @Override
  public void onResultsEnd(long timestamp)
  {
    if (mSearchRunning && isAdded())
      onSearchEnd();
  }

  @Override
  public void onCategorySelected(String category)
  {
    if (!TextUtils.isEmpty(category) && category.equals("cian "))
    {
      mCianCategorySelected = true;
      mToolbarController.setQuery(category);

      Statistics.INSTANCE.trackSponsoredEvent(Statistics.EventName.SEARCH_SPONSOR_CATEGORY_SELECTED,
                                              Sponsored.TYPE_CIAN);
      showAllResultsOnMap();
      mCianCategorySelected = false;
    }
    else
    {
      mToolbarController.setQuery(category);
    }
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
  }

  @Override
  public boolean onBackPressed()
  {
    if (mFilterController != null && mFilterController.onBackPressed())
      return true;
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
}
