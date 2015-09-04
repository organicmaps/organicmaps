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
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmFragment;
import com.mapswithme.maps.base.OnBackPressListener;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.sound.TtsPlayer;
import com.mapswithme.maps.widget.SearchToolbarController;
import com.mapswithme.util.InputUtils;
import com.mapswithme.util.Language;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.concurrency.UiThread;
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
  // Make 5-step increment to leave space for middle queries.
  // This constant should be equal with native SearchAdapter::QUERY_STEP;
  private static final int QUERY_STEP = 5;

  private static final int RC_VOICE_RECOGNITION = 0xCA11;
  private static final double DUMMY_NORTH = -1;

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
      mToolbar.setNavigationOnClickListener(
          new View.OnClickListener()
          {
            @Override
            public void onClick(View v)
            {
              if (getQuery().isEmpty())
              {
                Utils.navigateToParent(getActivity());
                return;
              }

              setQuery("");
              FloatingSearchToolbarController.cancelSearch();
              updateFrames();
            }
          });
    }

    @Override
    protected void onTextChanged(String query)
    {
      if (!isAdded())
        return;

      if (TextUtils.isEmpty(query))
      {
        mSearchAdapter.clear();
        nativeClearLastQuery();

        stopSearch();
        return;
      }

      // TODO: This code only for demonstration purposes and will be removed soon
      if (tryChangeMapStyle(query) || trySwitchOnTurnSound(query))
        return;

      runSearch(true);
    }

    @Override
    protected boolean onStartSearchClick()
    {
      if (!mSearchAdapter.showPopulateButton())
        return false;

      showAllResultsOnMap();
      return true;
    }

    @Override
    protected void onClearClick()
    {
      super.onClearClick();
      FloatingSearchToolbarController.cancelApiCall();
      FloatingSearchToolbarController.cancelSearch();
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
      if (TextUtils.isEmpty(getSearchQuery()))
      {
        super.onUpClick();
        return;
      }

      mToolbarController.clear();
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
        mToolbarController.setActive(false);
    }
  };

  private final CachedResults mCachedResults = new CachedResults(this);
  private final LastPosition mLastPosition = new LastPosition();

  private int mQueryId;
  private volatile boolean mSearchRunning;


  private boolean doShowDownloadSuggest()
  {
    return ActiveCountryTree.getTotalDownloadedCount() == 0;
  }

  private void showDownloadSuggest()
  {
    FragmentManager fm = getChildFragmentManager();
    String fragmentName = CountrySuggestFragment.class.getName();
    Fragment fragment = fm.findFragmentByTag(fragmentName);

    if (fragment == null || fragment.isDetached() || fragment.isRemoving())
    {
      fragment = Fragment.instantiate(getActivity(), fragmentName, null);
      fm.beginTransaction()
        .add(R.id.inner_fragment_container, fragment, fragmentName)
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
    boolean active = searchActive();
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
    boolean show = (!mSearchRunning &&
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
    new TabAdapter(getChildFragmentManager(), mPager, (TabLayout)root.findViewById(R.id.tabs));

    mResultsFrame = root.findViewById(R.id.results_frame);
    mResults = (RecyclerView) mResultsFrame.findViewById(R.id.recycler);
    setRecyclerScrollListener(mResults);
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

    mResults.setLayoutManager(new LinearLayoutManager(view.getContext()));
    mResults.setAdapter(mSearchAdapter);

    updateFrames();
    updateResultsPlaceholder();

    nativeConnectSearchListener();
    readArguments();
  }

  @Override
  public void onResume()
  {
    super.onResume();
    LocationHelper.INSTANCE.addLocationListener(this);
    mToolbarController.setActive(true);
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

    nativeDisconnectSearchListener();
    super.onDestroy();
  }

  protected String getSearchQuery()
  {
    return mToolbarController.getQuery();
  }

  protected void setSearchQuery(String text)
  {
    mToolbarController.setQuery(text);
  }

  protected boolean searchActive()
  {
    return !getSearchQuery().isEmpty();
  }

  private void readArguments()
  {
    Bundle arguments = getArguments();
    if (arguments == null)
      return;

    final String query = arguments.getString(SearchActivity.EXTRA_QUERY);
    if (query != null)
      setSearchQuery(query);
  }

  private void hideSearch()
  {
    mToolbarController.clear();
    mToolbarController.setActive(false);
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

    // close Search panel
    hideSearch();

    // change map style for the Map activity
    final int mapStyle = isOld ? Framework.MAP_STYLE_LIGHT : (isDark ? Framework.MAP_STYLE_DARK : Framework.MAP_STYLE_CLEAR);
    Framework.setMapStyle(mapStyle);

    return true;
  }

  private boolean trySwitchOnTurnSound(String query)
  {
    final boolean sound = "?sound".equals(query);
    final boolean nosound = "?nosound".equals(query);

    if (!sound && !nosound)
      return false;

    hideSearch();
    TtsPlayer.INSTANCE.enable(sound);

    return sound;
  }
  // FIXME END

  protected void showSingleResultOnMap(int resultIndex)
  {
    final String query = getSearchQuery();
    SearchRecentsTemp.INSTANCE.add(query);

    FloatingSearchToolbarController.cancelApiCall();
    FloatingSearchToolbarController.saveQuery("");
    nativeShowItem(resultIndex);
    Utils.navigateToParent(getActivity());
  }

  protected void showAllResultsOnMap()
  {
    final String query = getSearchQuery();
    nativeShowAllSearchResults();
    runInteractiveSearch(query, Language.getKeyboardLocale());
    FloatingSearchToolbarController.saveQuery(query);
    Utils.navigateToParent(getActivity());

    Statistics.INSTANCE.trackSimpleNamedEvent(Statistics.EventName.SEARCH_KEY_CLICKED);
  }

  @Override
  public void onLocationUpdated(final Location l)
  {
    mLastPosition.set(l.getLatitude(), l.getLongitude());

    if (!TextUtils.isEmpty(getSearchQuery()))
      runSearch(false);
  }

  @Override
  public void onCompassUpdated(long time, double magneticNorth, double trueNorth, double accuracy) {}

  @Override
  public void onLocationError(int errorCode) {}

  private void stopSearch()
  {
    mSearchRunning = false;
    mToolbarController.showProgress(false);
    updateFrames();
    updateResultsPlaceholder();
  }

  private void runSearch(boolean force)
  {
    int id = mQueryId + QUERY_STEP;
    if (!nativeRunSearch(getSearchQuery(), Language.getKeyboardLocale(), id, force, mLastPosition.valid, mLastPosition.lat, mLastPosition.lon))
      return;

    mSearchRunning = true;
    mQueryId = id;
    mToolbarController.showProgress(true);

    updateFrames();
  }

  @Override
  public void onResultsUpdate(final int count, final int queryId)
  {
    if (!mSearchRunning)
      return;

    UiThread.run(new Runnable()
    {
      @Override
      public void run()
      {
        if (!isAdded() || !searchActive())
          return;

        updateFrames();
        mSearchAdapter.refreshData(count, queryId);
      }
    });
  }

  @Override
  public void onResultsEnd(int queryId)
  {
    if (!mSearchRunning || queryId != mQueryId)
      return;

    UiThread.run(new Runnable()
    {
      @Override
      public void run()
      {
        if (!isAdded())
          return;

        stopSearch();
      }
    });
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
        setSearchQuery(result);
    }
  }

  @Override
  public boolean onBackPressed()
  {
    return false;
  }

  SearchResult getUncachedResult(int position, int queryId)
  {
    return nativeGetResult(position, queryId, mLastPosition.valid, mLastPosition.lat, mLastPosition.lon, DUMMY_NORTH);
  }

  public SearchResult getResult(int position, int queryId)
  {
    return mCachedResults.get(position, queryId);
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

  boolean isSearchRunning()
  {
    return mSearchRunning;
  }

  public static native void nativeShowItem(int position);

  public static native void nativeShowAllSearchResults();

  private static native SearchResult nativeGetResult(int position, int queryId, boolean hasPosition, double lat, double lon, double north);

  private native void nativeConnectSearchListener();

  private native void nativeDisconnectSearchListener();

  private native boolean nativeRunSearch(String s, String lang, int queryId, boolean force, boolean hasPosition, double lat, double lon);

  private native String nativeGetLastQuery();

  private native void nativeClearLastQuery();

  private native void runInteractiveSearch(String query, String lang);
}
