package com.mapswithme.maps.search;

import android.app.Activity;
import android.content.Intent;
import android.location.Location;
import android.os.Bundle;
import android.support.v7.widget.SearchView;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AbsListView;
import android.widget.ListView;

import com.mapswithme.maps.BuildConfig;
import com.mapswithme.maps.MWMActivity;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.MWMListFragment;
import com.mapswithme.maps.base.OnBackPressListener;
import com.mapswithme.maps.location.LocationService;
import com.mapswithme.util.InputUtils;
import com.mapswithme.util.Language;
import com.mapswithme.util.statistics.Statistics;


public class SearchFragment extends MWMListFragment implements View.OnClickListener, LocationService.LocationListener, OnBackPressListener
{
  // These constants should be equal with
  // Java_com_mapswithme_maps_SearchActivity_nativeRunSearch routine.
  private static final int NOT_FIRST_QUERY = 1;
  private static final int HAS_POSITION = 2;
  // Make 5-step increment to leave space for middle queries.
  // This constant should be equal with native SearchAdapter::QUERY_STEP;
  private final static int QUERY_STEP = 5;
  // These constants should be equal with search_params.hpp
  //
  private static final int AROUND_POSITION = 1;
  private static final int IN_VIEWPORT = 2;
  private static final int SEARCH_WORLD = 4;
  private static final int ALL = AROUND_POSITION | IN_VIEWPORT | SEARCH_WORLD;
  private int mSearchMode = ALL;
  //
  private static final int SEARCH_LAUNCHED = 0;
  private static final int QUERY_EMPTY = 1;
  private static final int SEARCH_SKIPPED = 2;
  // Handle voice recognition here
  private final static int RC_VOICE_RECOGNITION = 0xCA11;
  // Search views
  private SearchView mSearchView;
  // Current position.
  private double mLat;
  private double mLon;
  private double mNorth = -1.0;
  //
  private int mFlags = 0;
  private int mQueryId = 0;

  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState)
  {
    return inflater.inflate(R.layout.fragment_search, container, false);
  }

  @Override
  public void onViewCreated(View view, Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);

    setUpView((ViewGroup) view);
    // Create search list view adapter.
    setListAdapter(new SearchAdapter(this));

    nativeConnect();

    final Bundle args = getArguments();
    if (args != null)
    {
      mSearchView.setQuery(args.getString(SearchActivity.EXTRA_QUERY, ""), true);
      runSearch();
    }

//    mSearchEt.setOnEditorActionListener(new TextView.OnEditorActionListener()
//    {
//      @Override
//      public boolean onEditorAction(TextView v, int actionId, KeyEvent event)
//      {
//        final boolean isSearchDown = (event != null) &&
//            (event.getAction() == KeyEvent.ACTION_DOWN && event.getKeyCode() == KeyEvent.KEYCODE_SEARCH);
//        final boolean isActionSearch = (actionId == EditorInfo.IME_ACTION_SEARCH);
//
//        if ((isSearchDown || isActionSearch) && getSearchAdapter().getCount() > 0)
//        {
//          Statistics.INSTANCE.trackSimpleNamedEvent(Statistics.EventName.SEARCH_KEY_CLICKED);
//          if (!doShowCategories())
//            showSearchResultOnMap(0);
//          return true;
//        }
//        return false;
//      }
//    });
  }

  @Override
  public void onDestroy()
  {
    nativeDisconnect();
    super.onDestroy();
  }

  /**
   * If search string is empty - show search categories.
   */
  protected boolean doShowCategories()
  {
    return getSearchString().length() == 0;
  }

  private String getSearchString()
  {
    return mSearchView.getQuery().toString();
  }

  private void setUpView(ViewGroup root)
  {
    //    mVoiceInput = root.findViewById(R.id.search_voice_input);
    //    mSearchIcon = root.findViewById(R.id.search_icon);
    //    mSearchProgress = (ProgressBar) root.findViewById(R.id.search_progress);
    //    mClearQueryBtn = root.findViewById(R.id.search_image_clear);
    //    mClearQueryBtn.setOnClickListener(this);
    mSearchView = (SearchView) getToolbar().findViewById(R.id.search);

    // Initialize search edit box processor.
    //    mSearchEt = (EditText) root.findViewById(R.id.search_text_query);
    //    mSearchEt.addTextChangedListener(new TextWatcher()
    //    {
    //      @Override
    //      public void afterTextChanged(Editable s)
    //      {
    //        if (runSearch() == QUERY_EMPTY)
    //          showCategories();
    //
    //        if (s.length() == 0) // enable voice input
    //        {
    //          UiUtils.invisible(mClearQueryBtn);
    //          UiUtils.showIf(InputUtils.isVoiceInputSupported(getActivity()), mVoiceInput);
    //        }
    //        else // show clear cross
    //        {
    //          UiUtils.show(mClearQueryBtn);
    //          UiUtils.invisible(mVoiceInput);
    //        }
    //      }
    //
    //      @Override
    //      public void beforeTextChanged(CharSequence arg0, int arg1, int arg2, int arg3)
    //      {
    //      }
    //
    //      @Override
    //      public void onTextChanged(CharSequence s, int arg1, int arg2, int arg3)
    //      {
    //      }
    //    });

    //    mVoiceInput.setOnClickListener(this);

    final SearchView searchView = (SearchView) root.findViewById(R.id.search);
    searchView.setOnQueryTextListener(new SearchView.OnQueryTextListener()
    {
      @Override
      public boolean onQueryTextSubmit(String s)
      {
        Log.d("TEST", "Query text submit, " + s);
        mSearchView.clearFocus();
        return true;
      }

      @Override
      public boolean onQueryTextChange(String s)
      {
        Log.d("TEST", "Query text change, " + s);
        Statistics.INSTANCE.trackSimpleNamedEvent(Statistics.EventName.SEARCH_KEY_CLICKED);
        if (!doShowCategories())
          showSearchResultOnMap(0);
        return true;
      }
    });
    searchView.setIconified(false);
    getToolbar().setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        getActivity().getSupportFragmentManager().popBackStack();
      }
    });

    getListView().setOnScrollListener(new AbsListView.OnScrollListener() {
      @Override
      public void onScrollStateChanged(AbsListView view, int scrollState)
      {
        mSearchView.clearFocus();
        InputUtils.hideKeyboard(mSearchView);
      }

      @Override
      public void onScroll(AbsListView view, int firstVisibleItem, int visibleItemCount, int totalItemCount)
      {

      }
    });
  }

  @Override
  public void onResume()
  {
    super.onResume();

    // Reset current mode flag - start first search.
    mFlags = 0;
    mNorth = -1.0;

    LocationService.INSTANCE.startUpdate(this);

    // do the search immediately after resume
    // TODO
    //    Utils.setTextAndCursorToEnd(mSearchView, getLastQuery());
    //    mSearchEt.requestFocus();
  }

  @Override
  public void onPause()
  {
    LocationService.INSTANCE.stopUpdate(this);

    super.onPause();
  }

  @Override
  public void onListItemClick(ListView l, View v, int position, long id)
  {
    super.onListItemClick(l, v, position, id);
    final String suggestion = getSearchAdapter().onItemClick(position);
    if (suggestion == null)
      showSearchResultOnMap(position);
    else
      // set suggestion string and run search (this call invokes runSearch)
      runSearch(suggestion);
  }

  private void showSearchResultOnMap(int position)
  {
    // If user searched for something, clear API layer
    SearchController.getInstance().cancelApiCall();

    if (BuildConfig.IS_PRO)
    {
      // Put query string for "View on map" or feature name for search result.
      final boolean allResults = (position == 0);
      final String query = getSearchString();
      SearchController.getInstance().setQuery(allResults ? query : "");
      if (allResults)
      {
        nativeShowAllSearchResults();
        runInteractiveSearch(query, Language.getKeyboardInput(getActivity()));
      }

      MWMActivity.startWithSearchResult(getActivity(), !allResults);
      getActivity().onBackPressed();
    }
  }

  private SearchAdapter getSearchAdapter()
  {
    return (SearchAdapter) getListView().getAdapter();
  }

  @Override
  public boolean onBackPressed()
  {
    SearchController.getInstance().cancel();
    return false;
  }

  private void showCategories()
  {
    clearLastQuery();
    setSearchInProgress(false);
    getSearchAdapter().updateCategories();
  }

  @Override
  public void onLocationUpdated(final Location l)
  {
    mFlags |= HAS_POSITION;

    mLat = l.getLatitude();
    mLon = l.getLongitude();

    if (runSearch() == SEARCH_SKIPPED)
      getSearchAdapter().updateData();
  }

  @Override
  public void onCompassUpdated(long time, double magneticNorth, double trueNorth, double accuracy) {}

  @Override
  public void onLocationError(int errorCode) {}

  private boolean isCurrentResult(int id)
  {
    return (id >= mQueryId && id < mQueryId + QUERY_STEP);
  }

  // Called from native code
  @SuppressWarnings("unused")
  public void updateData(final int count, final int resultID)
  {
    getActivity().runOnUiThread(new Runnable()
    {
      @Override
      public void run()
      {
        // if this results for the last query - hide progress
        if (isCurrentResult(resultID))
          setSearchInProgress(false);

        if (!doShowCategories())
        {
          // update list view with results if we are not in fragment_bookmark_categories mode
          getSearchAdapter().updateData(count, resultID);

          // scroll list view to the top
          setSelection(0);
        }
      }
    });
  }

  private void runSearch(String s)
  {
    // TODO
    //    Utils.setTextAndCursorToEnd(mSearchEt, s);
    mSearchView.setQuery(s, true);
    mSearchView.requestFocus();
  }

  private int runSearch()
  {
    final String s = getSearchString();
    if (s.length() == 0)
    {
      // search next time from categories list
      mFlags &= (~NOT_FIRST_QUERY);
      return QUERY_EMPTY;
    }

    final int id = mQueryId + QUERY_STEP;
    if (nativeRunSearch(s, Language.getKeyboardInput(getActivity()), mLat, mLon, mFlags, mSearchMode, id))
    {
      // store current query
      mQueryId = id;

      // mark that it's not the first query already - don't do force search
      mFlags |= NOT_FIRST_QUERY;

      setSearchInProgress(true);

      return SEARCH_LAUNCHED;
    }
    else
      return SEARCH_SKIPPED;
  }

  private void setSearchInProgress(boolean inProgress)
  {
    // TODO show progress?
    if (inProgress)
    {
      //      UiUtils.show(mSearchProgress);
      //      UiUtils.invisible(mSearchIcon);
    }
    else // search is completed
    {
      //      UiUtils.invisible(mSearchProgress);
      //      UiUtils.show(mSearchIcon);
    }
  }

  @Override
  public void onClick(View v)
  {
    switch (v.getId())
    {
    case R.id.btn_cancel_search:
      SearchController.getInstance().cancel();
      clearLastQuery();
      getActivity().onBackPressed();
      break;
    case R.id.search_image_clear:
      // TODO
      break;
    case R.id.search_voice_input:
      final Intent vrIntent = InputUtils.createIntentForVoiceRecognition(getResources().getString(R.string.search_map));
      startActivityForResult(vrIntent, RC_VOICE_RECOGNITION);
      break;
    }
  }

  public SearchAdapter.SearchResult getResult(int position, int queryID)
  {
    return nativeGetResult(position, queryID, mLat, mLon, (mFlags & HAS_POSITION) != 0, mNorth);
  }

  @Override
  public void onActivityResult(int requestCode, int resultCode, Intent data)
  {
    super.onActivityResult(requestCode, resultCode, data);

    if ((requestCode == RC_VOICE_RECOGNITION) && (resultCode == Activity.RESULT_OK))
    {
      final String result = InputUtils.getMostConfidentResult(data);
      if (result != null)
        mSearchView.setQuery(result, true);
    }
  }

  public static native void nativeShowItem(int position);

  public static native void nativeShowAllSearchResults();

  private static native SearchAdapter.SearchResult nativeGetResult(int position, int queryID,
                                                                   double lat, double lon, boolean mode, double north);

  private native void nativeConnect();

  private native void nativeDisconnect();

  private native boolean nativeRunSearch(String s, String lang, double lat, double lon, int flags,
                                         int searchMode, int queryID);

  private native String getLastQuery();

  private native void clearLastQuery();

  private native void runInteractiveSearch(String query, String lang);
}
