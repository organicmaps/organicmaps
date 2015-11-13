package com.mapswithme.maps.search;

import android.app.Activity;
import android.content.ActivityNotFoundException;
import android.content.Intent;
import android.location.Location;
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
import com.mapswithme.country.ActiveCountryTree;
import com.mapswithme.country.CountrySuggestFragment;
import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MwmActivity;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmFragment;
import com.mapswithme.maps.base.OnBackPressListener;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.routing.RoutingController;
import com.mapswithme.maps.widget.SearchToolbarController;
import com.mapswithme.util.InputUtils;
import com.mapswithme.util.Language;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.statistics.AlohaHelper;
import com.mapswithme.util.statistics.Statistics;

import java.util.ArrayList;
import java.util.List;


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
    protected void onUpClick()
    {
      if (TextUtils.isEmpty(getQuery()))
      {
        if (mFromRoutePlan)
          RoutingController.get().onPoiSelected(null);
        else
          super.onUpClick();

        mToolbarController.deactivate();
        return;
      }

      clear();
    }
  }

  private View mResultsFrame;
  private RecyclerView mResults;
  private View mResultsPlaceholder;

  private View mTabsFrame;

  private SearchToolbarController mToolbarController;
  private ViewPager mPager;

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
  private boolean mFromRoutePlan;

  private boolean doShowDownloadSuggest()
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

    ViewGroup root = (ViewGroup) view;
    mTabsFrame = root.findViewById(R.id.tab_frame);
    mPager = (ViewPager) mTabsFrame.findViewById(R.id.pages);
    mToolbarController = new ToolbarController(view);
    final TabAdapter tabAdapter = new TabAdapter(getChildFragmentManager(), mPager, (TabLayout) root.findViewById(R.id.tabs));

    mResultsFrame = root.findViewById(R.id.results_frame);
    mResults = (RecyclerView) mResultsFrame.findViewById(R.id.recycler);
    setRecyclerScrollListener(mResults);
    mResultsPlaceholder = mResultsFrame.findViewById(R.id.placeholder);

    readArguments();

    if (mSearchAdapter == null)
    {
      mSearchAdapter = new SearchAdapter(this, mFromRoutePlan);
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

    SearchEngine.INSTANCE.addListener(this);

    if (SearchRecents.getSize() == 0)
      mPager.setCurrentItem(TabAdapter.Tab.CATEGORIES.ordinal());

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

    final String query = arguments.getString(SearchActivity.EXTRA_QUERY);
    if (query != null)
      setQuery(query);

    mFromRoutePlan = arguments.getBoolean(SearchActivity.EXTRA_FROM_ROUTE_PLAN);

    mToolbarController.activate();
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

  void showSingleResultOnMap(int resultIndex)
  {
    final String query = getQuery();
    SearchRecents.add(query);
    SearchEngine.cancelApiCall();
    SearchEngine.showResult(query, resultIndex);
    Utils.navigateToParent(getActivity());

    Statistics.INSTANCE.trackSimpleNamedEvent(Statistics.EventName.SEARCH_KEY_CLICKED);
  }

  void showAllResultsOnMap()
  {
    final String query = getQuery();
    SearchRecents.add(query);
    mLastQueryTimestamp = System.nanoTime();
    SearchEngine.runInteractiveSearch(query, Language.getKeyboardLocale(),
            mLastQueryTimestamp, false /* isMapAndTable */);
    SearchEngine.showAllResults(query);
    Utils.navigateToParent(getActivity());

    Statistics.INSTANCE.trackSimpleNamedEvent(Statistics.EventName.SEARCH_ON_MAP_CLICKED);
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
    if (!searchActive())
    {
      if (mFromRoutePlan)
      {
        RoutingController.get().onPoiSelected(null);
        return true;
      }
      return false;
    }

    mToolbarController.clear();
    return true;
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
