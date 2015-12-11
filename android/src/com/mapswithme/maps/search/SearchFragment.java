package com.mapswithme.maps.search;

import android.app.Activity;
import android.content.ActivityNotFoundException;
import android.content.Intent;
import android.location.Location;
import android.os.Build;
import android.os.Bundle;
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

import java.util.ArrayList;
import java.util.List;

import com.mapswithme.country.ActiveCountryTree;
import com.mapswithme.country.CountrySuggestFragment;
import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MwmActivity;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmFragment;
import com.mapswithme.maps.base.OnBackPressListener;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.routing.RoutingController;
import com.mapswithme.maps.widget.SearchToolbarController;
import com.mapswithme.util.InputUtils;
import com.mapswithme.util.Language;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.statistics.AlohaHelper;
import com.mapswithme.util.statistics.Statistics;


public class SearchFragment extends BaseMwmFragment
                         implements LocationHelper.LocationListener,
                                    OnBackPressListener,
                                    NativeSearchListener,
                                    SearchToolbarController.Container,
                                    CategoriesAdapter.OnCategorySelectedListener
{
  private static final int RC_VOICE_RECOGNITION = 0xCA11;
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

      // TODO: This code only for demonstration purposes and will be removed soon
      if (tryChangeMapStyle(query))
        return;

      runSearch();
    }

    @Override
    protected boolean onStartSearchClick()
    {
      if (!mFromRoutePlan)
        showAllResultsOnMap();
      return true;
    }

    @Override
    protected void onVoiceInputClick()
    {
      try
      {
        startActivityForResult(InputUtils.createIntentForVoiceRecognition(getString(R.string.search_map)), RC_VOICE_RECOGNITION);
      } catch (ActivityNotFoundException e)
      {
        AlohaHelper.logException(e);
      }
    }

    @Override
    public void onUpClick()
    {
      if (!onBackPressed())
        super.onUpClick();
    }
  }

  private View mResultsFrame;
  private View mResultsPlaceholder;

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
  private String mInitialQuery;
  private boolean mFromRoutePlan;

  private static boolean doShowDownloadSuggest()
  {
    return ActiveCountryTree.getTotalDownloadedCount() == 0;
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
             .commit();
  }

  protected void updateFrames()
  {
    final boolean active = searchActive();
    UiUtils.showIf(active, mResultsFrame);

    if (active)
      hideDownloadSuggest();
    else if (doShowDownloadSuggest())
      showDownloadSuggest();
    else
      hideDownloadSuggest();
  }

  private void updateResultsPlaceholder()
  {
    final boolean show = (!mSearchRunning &&
                          mSearchAdapter.getItemCount() == 0 &&
                          searchActive());

    UiUtils.showIf(show, mResultsPlaceholder);
  }

  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState)
  {
    return inflater.inflate(R.layout.fragment_search, container, false);
  }

  @Override
  public void onViewCreated(View view, Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    readArguments();

    ViewGroup root = (ViewGroup) view;
    View tabsFrame = root.findViewById(R.id.tab_frame);
    ViewPager pager = (ViewPager) tabsFrame.findViewById(R.id.pages);

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP)
      UiUtils.hide(tabsFrame.findViewById(R.id.tabs_divider));

    mToolbarController = new ToolbarController(view);

    final TabAdapter tabAdapter = new TabAdapter(getChildFragmentManager(), pager, (TabLayout) root.findViewById(R.id.tabs));

    mResultsFrame = root.findViewById(R.id.results_frame);
    RecyclerView results = (RecyclerView) mResultsFrame.findViewById(R.id.recycler);
    setRecyclerScrollListener(results);
    mResultsPlaceholder = mResultsFrame.findViewById(R.id.placeholder);

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

    results.setLayoutManager(new LinearLayoutManager(view.getContext()));
    results.setAdapter(mSearchAdapter);

    updateFrames();
    updateResultsPlaceholder();

    if (mInitialQuery != null)
      setQuery(mInitialQuery);
    mToolbarController.activate();

    SearchEngine.INSTANCE.addListener(this);

    if (SearchRecents.getSize() == 0)
      pager.setCurrentItem(TabAdapter.Tab.CATEGORIES.ordinal());

    tabAdapter.setTabSelectedListener(new TabAdapter.OnTabSelectedListener()
    {
      @Override
      public void onTabSelected(TabAdapter.Tab tab)
      {
        mToolbarController.deactivate();
      }
    });
  }

  @Override
  public void onResume()
  {
    super.onResume();
    LocationHelper.INSTANCE.addLocationListener(this);
  }

  @Override
  public void onPause()
  {
    LocationHelper.INSTANCE.removeLocationListener(this);
    super.onPause();
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

  private boolean searchActive()
  {
    return !getQuery().isEmpty();
  }

  private void readArguments()
  {
    final Bundle arguments = getArguments();
    if (arguments == null)
      return;

    mInitialQuery = arguments.getString(SearchActivity.EXTRA_QUERY);
    mFromRoutePlan = RoutingController.get().isWaitingPoiPick();
  }

  private void hideSearch()
  {
    mToolbarController.clear();
    mToolbarController.deactivate();
    Utils.navigateToParent(getActivity());
  }

  // FIXME: This code only for demonstration purposes and will be removed soon
  private boolean tryChangeMapStyle(String str)
  {
    // Hook for shell command on change map style
    final boolean isDark = str.equals("mapstyle:dark") || str.equals("?dark");
    final boolean isLight = isDark ? false : str.equals("mapstyle:light") || str.equals("?light");
    final boolean isOld = isDark || isLight ? false : str.equals("?oldstyle");

    if (!isDark && !isLight && !isOld)
      return false;

    hideSearch();

    // change map style for the Map activity
    final int mapStyle = isOld ? Framework.MAP_STYLE_LIGHT : (isDark ? Framework.MAP_STYLE_DARK : Framework.MAP_STYLE_CLEAR);
    Framework.setMapStyle(mapStyle);

    return true;
  }
  // FIXME END

  private void processSelected(SearchResult result)
  {
    if (mFromRoutePlan)
    {
      //noinspection ConstantConditions
      final MapObject point = new MapObject.SearchResult(result.name, result.description.featureType, result.lat, result.lon);
      RoutingController.get().onPoiSelected(point);
    }

    mToolbarController.deactivate();

    if (getActivity() instanceof SearchActivity)
      Utils.navigateToParent(getActivity());
  }

  void showSingleResultOnMap(SearchResult result, int resultIndex)
  {
    final String query = getQuery();
    SearchRecents.add(query);
    SearchEngine.cancelApiCall();

    if (!mFromRoutePlan)
      SearchEngine.showResult(resultIndex);

    processSelected(result);

    Statistics.INSTANCE.trackEvent(Statistics.EventName.SEARCH_ITEM_CLICKED);
  }

  void showAllResultsOnMap()
  {
    final String query = getQuery();
    SearchRecents.add(query);
    mLastQueryTimestamp = System.nanoTime();
    SearchEngine.runInteractiveSearch(query, Language.getKeyboardLocale(), mLastQueryTimestamp, false /* isMapAndTable */);
    SearchEngine.showAllResults(query);
    Utils.navigateToParent(getActivity());

    Statistics.INSTANCE.trackEvent(Statistics.EventName.SEARCH_ON_MAP_CLICKED);
  }

  @Override
  public void onLocationUpdated(final Location l)
  {
    mLastPosition.set(l.getLatitude(), l.getLongitude());

    if (!TextUtils.isEmpty(getQuery()))
      mSearchAdapter.notifyDataSetChanged();
  }

  @Override
  public void onCompassUpdated(long time, double magneticNorth, double trueNorth, double accuracy) {}

  @Override
  public void onLocationError(int errorCode) {}

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
    SearchEngine.cancelSearch();
    onSearchEnd();
  }

  private void runSearch()
  {
    mLastQueryTimestamp = System.nanoTime();
    // TODO @yunitsky Implement more elegant solution.
    if (getActivity() instanceof MwmActivity)
    {
      SearchEngine.runInteractiveSearch(getQuery(), Language.getKeyboardLocale(),
              mLastQueryTimestamp, true /* isMapAndTable */);
    }
    else
    {
      final boolean searchStarted = SearchEngine.runSearch(getQuery(), Language.getKeyboardLocale(), mLastQueryTimestamp, true,
                                                           mLastPosition.valid, mLastPosition.lat, mLastPosition.lon);
      if (!searchStarted)
        return;
    }

    mSearchRunning = true;
    mToolbarController.showProgress(true);

    updateFrames();
  }

  @Override
  public void onResultsUpdate(SearchResult[] results, long timestamp)
  {
    if (!isAdded() || !searchActive())
      return;

    // Search is running hence results updated.
    mSearchRunning = true;
    updateFrames();
    mSearchAdapter.refreshData(results);
    mToolbarController.showProgress(true);
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
    mToolbarController.setQuery(category);
  }

  @Override
  public void onActivityResult(int requestCode, int resultCode, Intent data)
  {
    super.onActivityResult(requestCode, resultCode, data);

    if (requestCode == RC_VOICE_RECOGNITION && resultCode == Activity.RESULT_OK)
    {
      String result = InputUtils.getBestRecognitionResult(data);
      if (!TextUtils.isEmpty(result))
        setQuery(result);
    }
  }

  @Override
  public boolean onBackPressed()
  {
    if (searchActive())
    {
      mToolbarController.clear();
      return true;
    }

    mToolbarController.deactivate();
    if (mFromRoutePlan)
    {
      RoutingController.get().onPoiSelected(null);
      return !(getActivity() instanceof SearchActivity);
    }

    return false;
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
