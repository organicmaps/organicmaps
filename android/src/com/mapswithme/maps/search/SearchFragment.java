package com.mapswithme.maps.search;

import android.app.Activity;
import android.content.Intent;
import android.location.Location;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.EditorInfo;
import android.widget.AbsListView;
import android.widget.AbsListView.OnScrollListener;
import android.widget.EditText;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.TextView;

import com.mapswithme.maps.MWMActivity;
import com.mapswithme.maps.Framework;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmListFragment;
import com.mapswithme.maps.base.OnBackPressListener;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.util.InputUtils;
import com.mapswithme.util.Language;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.statistics.Statistics;


public class SearchFragment extends BaseMwmListFragment implements View.OnClickListener, LocationHelper.LocationListener, OnBackPressListener
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
  private EditText mSearchEt;
  private ProgressBar mSearchProgress;
  private View mClearQueryBtn;
  private View mVoiceInput;
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
      mSearchEt.setText(args.getString(SearchActivity.EXTRA_QUERY));
      runSearch();
    }

    mSearchEt.setOnEditorActionListener(new TextView.OnEditorActionListener()
    {
      @Override
      public boolean onEditorAction(TextView v, int actionId, KeyEvent event)
      {
        final boolean isSearchDown = (event != null) &&
            (event.getAction() == KeyEvent.ACTION_DOWN && event.getKeyCode() == KeyEvent.KEYCODE_SEARCH);
        final boolean isActionSearch = (actionId == EditorInfo.IME_ACTION_SEARCH);

        if ((isSearchDown || isActionSearch) && getSearchAdapter().getCount() > 0)
        {
          Statistics.INSTANCE.trackSimpleNamedEvent(Statistics.EventName.SEARCH_KEY_CLICKED);
          if (!doShowCategories())
            showSearchResultOnMap(0);
          return true;
        }
        return false;
      }
    });
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
    return mSearchEt.getText().toString();
  }

  // TODO: This code only for demonstration purposes and will be removed soon
  private boolean tryChangeMapStyleCmd(String str)
  {
    // Hook for shell command on change map style
    final boolean isDark = str.equals("mapstyle:dark");
    final boolean isLight = isDark ? false : str.equals("mapstyle:light");

    if (!isDark && !isLight)
      return false;

    // close Search panel
    mSearchEt.setText(null);
    InputUtils.hideKeyboard(mSearchEt);
    getActivity().onBackPressed();

    // change map style for the Map activity
    final int mapStyle = isDark ? Framework.MAP_STYLE_DARK : Framework.MAP_STYLE_LIGHT;
    MWMActivity.setMapStyle(getActivity(), mapStyle);

    return true;
  }

  private void setUpView(ViewGroup root)
  {
    mVoiceInput = root.findViewById(R.id.search_voice_input);
    mSearchProgress = (ProgressBar) root.findViewById(R.id.search_progress);
    mClearQueryBtn = root.findViewById(R.id.search_image_clear);
    mClearQueryBtn.setOnClickListener(this);

    // Initialize search edit box processor.
    mSearchEt = (EditText) root.findViewById(R.id.search_text_query);
    mSearchEt.addTextChangedListener(new TextWatcher()
    {
      @Override
      public void afterTextChanged(Editable s)
      {
        // TODO: This code only for demonstration purposes and will be removed soon
        if (tryChangeMapStyleCmd(s.toString()))
          return;

        if (runSearch() == QUERY_EMPTY)
          showCategories();

        if (s.length() == 0) // enable voice input
        {
          UiUtils.invisible(mClearQueryBtn);
          UiUtils.showIf(InputUtils.isVoiceInputSupported(getActivity()), mVoiceInput);
        }
        else // show clear cross
        {
          UiUtils.show(mClearQueryBtn);
          UiUtils.invisible(mVoiceInput);
        }
      }

      @Override
      public void beforeTextChanged(CharSequence arg0, int arg1, int arg2, int arg3)
      {
      }

      @Override
      public void onTextChanged(CharSequence s, int arg1, int arg2, int arg3)
      {
      }
    });

    mVoiceInput.setOnClickListener(this);

    getListView().setOnScrollListener(new OnScrollListener()
    {
      @Override
      public void onScrollStateChanged(AbsListView view, int scrollState)
      {
        // Hide keyboard when user starts scroll
        InputUtils.hideKeyboard(mSearchEt);

        // Hacky way to remove focus from only edittext at activity
        mSearchEt.setFocusableInTouchMode(false);
        mSearchEt.setFocusable(false);
        mSearchEt.setFocusableInTouchMode(true);
        mSearchEt.setFocusable(true);
      }

      @Override
      public void onScroll(AbsListView view, int firstVisibleItem, int visibleItemCount, int totalItemCount)
      {}
    });

    getToolbar().setNavigationOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        getActivity().onBackPressed();
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

    LocationHelper.INSTANCE.addLocationListener(this);

    // do the search immediately after resume
    Utils.setTextAndCursorToEnd(mSearchEt, getLastQuery());
    mSearchEt.requestFocus();
  }

  @Override
  public void onPause()
  {
    LocationHelper.INSTANCE.removeLocationListener(this);

    super.onPause();
  }

  @Override
  public void onListItemClick(ListView l, View v, int position, long id)
  {
    super.onListItemClick(l, v, position, id);

    if (!isAdded())
      return;

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

    // Put query string for "View on map" or feature name for search result.
    final boolean allResults = (position == 0);
    final String query = getSearchString();
    SearchController.getInstance().setQuery(allResults ? query : "");
    if (allResults)
    {
      nativeShowAllSearchResults();
      runInteractiveSearch(query, Language.getKeyboardInput(getActivity()));
    }

    InputUtils.hideKeyboard(mSearchEt);
    MWMActivity.startWithSearchResult(getActivity(), !allResults);
    getActivity().getSupportFragmentManager().popBackStack();
  }

  private SearchAdapter getSearchAdapter()
  {
    return (SearchAdapter) getListView().getAdapter();
  }

  @Override
  public boolean onBackPressed()
  {
    SearchController.getInstance().cancelSearch();
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
        if (!isAdded())
          return;

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
    Utils.setTextAndCursorToEnd(mSearchEt, s);
  }

  private int runSearch()
  {
    final String s = getSearchString();
    if (s.length() == 0)
    {
      // do force search next time from fragment_bookmark_categories list
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
    if (inProgress)
      UiUtils.show(mSearchProgress);
    else // search is completed
      UiUtils.invisible(mSearchProgress);
  }

  @Override
  public void onClick(View v)
  {
    switch (v.getId())
    {
    case R.id.search_image_clear:
      mSearchEt.setText(null);
      SearchController.getInstance().cancelApiCall();
      SearchController.getInstance().cancelSearch();
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
        mSearchEt.setText(result);
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
