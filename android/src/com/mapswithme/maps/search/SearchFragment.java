package com.mapswithme.maps.search;

import android.app.Activity;
import android.content.Intent;
import android.location.Location;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.Log;
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

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MWMActivity;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmListFragment;
import com.mapswithme.maps.base.OnBackPressListener;
import com.mapswithme.maps.data.RouterTypes;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.util.InputUtils;
import com.mapswithme.util.Language;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.statistics.Statistics;


public class SearchFragment extends BaseMwmListFragment implements View.OnClickListener, LocationHelper.LocationListener, OnBackPressListener
{
  // These constants should be equal with
  // Java_com_mapswithme_maps_SearchFragment_nativeRunSearch routine.
  private static final int NOT_FIRST_QUERY = 1;
  private static final int HAS_POSITION = 2;
  // Make 5-step increment to leave space for middle queries.
  // This constant should be equal with native SearchAdapter::QUERY_STEP;
  private final static int QUERY_STEP = 5;
  //
  private static final int STATUS_SEARCH_LAUNCHED = 0;
  private static final int STATUS_QUERY_EMPTY = 1;
  private static final int STATUS_SEARCH_SKIPPED = 2;
  // Handle voice recognition here
  private final static int RC_VOICE_RECOGNITION = 0xCA11;
  // Search views
  private EditText mEtSearchQuery;
  private ProgressBar mPbSearch;
  private View mBtnClearQuery;
  private View mBtnVoice;
  // Current position.
  private double mLat;
  private double mLon;
  private double mNorth = -1.0;
  //
  private int mFlags = 0;
  private int mQueryId = 0;
  private SearchAdapter mAdapter;

  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState)
  {
    return inflater.inflate(R.layout.fragment_search, container, false);
  }

  @Override
  public void onViewCreated(View view, Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);

    final ViewGroup root = (ViewGroup) view;
    setUpView(root);
    // Create search list view adapter & add header
    initAdapter();
    setListAdapter(mAdapter);

    nativeConnect();

    final Bundle args = getArguments();
    if (args != null)
      setSearchQuery(args.getString(SearchActivity.EXTRA_QUERY));
  }

  private void initAdapter()
  {
    mAdapter = new SearchAdapter(this);
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
    return mEtSearchQuery.getText().toString();
  }

  private void setUpView(ViewGroup root)
  {
    mBtnVoice = root.findViewById(R.id.search_voice_input);
    mPbSearch = (ProgressBar) root.findViewById(R.id.search_progress);
    mBtnClearQuery = root.findViewById(R.id.search_image_clear);
    mBtnClearQuery.setOnClickListener(this);

    // Initialize search edit box processor.
    mEtSearchQuery = (EditText) root.findViewById(R.id.search_text_query);
    mEtSearchQuery.addTextChangedListener(new TextWatcher()
    {
      @Override
      public void afterTextChanged(Editable s)
      {
        // TODO: This code only for demonstration purposes and will be removed soon
        if (tryChangeMapStyle(s.toString()) ||
            tryChangeRouter(s.toString()))
          return;

        if (runSearch() == STATUS_QUERY_EMPTY)
          showCategories();

        if (s.length() == 0)
        {
          UiUtils.invisible(mBtnClearQuery);
          UiUtils.showIf(InputUtils.isVoiceInputSupported(getActivity()), mBtnVoice);
        }
        else
        {
          UiUtils.show(mBtnClearQuery);
          UiUtils.invisible(mBtnVoice);
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

    mEtSearchQuery.setOnEditorActionListener(new TextView.OnEditorActionListener()
    {
      @Override
      public boolean onEditorAction(TextView v, int actionId, KeyEvent event)
      {
        final boolean isSearchDown = (event != null) &&
            (event.getAction() == KeyEvent.ACTION_DOWN && event.getKeyCode() == KeyEvent.KEYCODE_SEARCH);
        final boolean isActionSearch = (actionId == EditorInfo.IME_ACTION_SEARCH);

        if ((isSearchDown || isActionSearch) && mAdapter.getCount() > 0)
        {
          Statistics.INSTANCE.trackSimpleNamedEvent(Statistics.EventName.SEARCH_KEY_CLICKED);
          if (!doShowCategories())
            showSearchResultOnMap(0);
          return true;
        }
        return false;
      }
    });

    mBtnVoice.setOnClickListener(this);

    final ListView listView = getListView();
    listView.setOnScrollListener(new OnScrollListener()
    {
      @Override
      public void onScrollStateChanged(AbsListView view, int scrollState)
      {
        // Hide keyboard when user starts scroll
        InputUtils.hideKeyboard(mEtSearchQuery);

        // Hacky way to remove focus from only edittext at activity
        mEtSearchQuery.setFocusableInTouchMode(false);
        mEtSearchQuery.setFocusable(false);
        mEtSearchQuery.setFocusableInTouchMode(true);
        mEtSearchQuery.setFocusable(true);
      }

      @Override
      public void onScroll(AbsListView view, int firstVisibleItem, int visibleItemCount, int totalItemCount)
      {}
    });

    listView.addHeaderView(getActivity().getLayoutInflater().inflate(R.layout.header_default, listView, false), null, false);
  }


  // FIXME: This code only for demonstration purposes and will be removed soon
  private boolean tryChangeMapStyle(String str)
  {
    // Hook for shell command on change map style
    final boolean isDark = str.equals("mapstyle:dark");
    final boolean isLight = isDark ? false : str.equals("mapstyle:light");

    if (!isDark && !isLight)
      return false;

    // close Search panel
    mEtSearchQuery.setText(null);
    InputUtils.hideKeyboard(mEtSearchQuery);
    getActivity().onBackPressed();

    // change map style for the Map activity
    final int mapStyle = isDark ? Framework.MAP_STYLE_DARK : Framework.MAP_STYLE_LIGHT;
    MWMActivity.setMapStyle(getActivity(), mapStyle);

    return true;
  }

  private boolean tryChangeRouter(String query)
  {
    final boolean pedestrian = query.equals("pedestrian:on");
    final boolean wehicle = query.equals("pedestrian:off");

    if (!pedestrian && !wehicle)
      return false;

    mEtSearchQuery.setText(null);
    InputUtils.hideKeyboard(mEtSearchQuery);
    getActivity().onBackPressed();

    RouterTypes.saveRouterType(pedestrian ? RouterTypes.ROUTER_PEDESTRIAN : RouterTypes.ROUTER_VEHICLE);

    return false;
  }
  // FIXME: This code only for demonstration purposes and will be removed soon


  @Override
  public void onResume()
  {
    super.onResume();

    // Reset current mode flag - start first search.
    mFlags = 0;
    mNorth = -1.0;

    LocationHelper.INSTANCE.addLocationListener(this);

    setSearchQuery(getLastQuery());
    mEtSearchQuery.requestFocus();
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

    position -= l.getHeaderViewsCount();
    final String suggestion = mAdapter.onItemClick(position);
    if (suggestion == null)
      showSearchResultOnMap(position);
    else
      // set suggestion string and run search (this call invokes runSearch)
      setSearchQuery(suggestion);
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

    InputUtils.hideKeyboard(mEtSearchQuery);
    MWMActivity.startWithSearchResult(getActivity(), !allResults);
    getActivity().getSupportFragmentManager().popBackStack();
  }

  @Override
  public boolean onBackPressed()
  {
    SearchController.getInstance().cancelSearch();
    return false;
  }

  private void showCategories()
  {
    // TODO clear edittexty?
    clearLastQuery();
    displaySearchProgress(false);
    mAdapter.updateCategories();
  }

  @Override
  public void onLocationUpdated(final Location l)
  {
    mFlags |= HAS_POSITION;

    mLat = l.getLatitude();
    mLon = l.getLongitude();

    if (runSearch() == STATUS_SEARCH_SKIPPED)
      mAdapter.notifyDataSetChanged();
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
  public void updateData(final int count, final int resultId)
  {
    getActivity().runOnUiThread(new Runnable()
    {
      @Override
      public void run()
      {
        if (!isAdded())
          return;

        if (!doShowCategories())
        {
          mAdapter.updateData(count, resultId);
          // scroll list view to the top
          setSelection(0);
        }
      }
    });
  }

  // Called from native code
  @SuppressWarnings("unused")
  public void endData()
  {
    getActivity().runOnUiThread(new Runnable()
    {
      @Override
      public void run()
      {
        if (!isAdded())
          return;

        displaySearchProgress(false);
      }
    });
  }

  private void setSearchQuery(String query)
  {
    Utils.setTextAndCursorToEnd(mEtSearchQuery, query);
  }

  private int runSearch()
  {
    final String query = getSearchString();
    if (query.isEmpty())
    {
      // do force search next time from empty list
      mFlags &= (~NOT_FIRST_QUERY);
      return STATUS_QUERY_EMPTY;
    }

    final int id = mQueryId + QUERY_STEP;
    if (nativeRunSearch(query, Language.getKeyboardInput(getActivity()), mLat, mLon, mFlags, id))
    {
      mQueryId = id;
      // mark that it's not the first query already - don't do force search
      mFlags |= NOT_FIRST_QUERY;
      displaySearchProgress(true);
      mAdapter.notifyDataSetChanged();
      return STATUS_SEARCH_LAUNCHED;
    }
    else
      return STATUS_SEARCH_SKIPPED;
  }

  private void displaySearchProgress(boolean inProgress)
  {
    if (inProgress)
      UiUtils.show(mPbSearch);
    else // search is completed
      UiUtils.invisible(mPbSearch);
  }

  @Override
  public void onClick(View v)
  {
    switch (v.getId())
    {
    case R.id.search_image_clear:
      mEtSearchQuery.setText(null);
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
        mEtSearchQuery.setText(result);
    }
  }

  public static native void nativeShowItem(int position);

  public static native void nativeShowAllSearchResults();

  private static native SearchAdapter.SearchResult nativeGetResult(int position, int queryId,
                                                                   double lat, double lon, boolean mode, double north);

  private native void nativeConnect();

  private native void nativeDisconnect();

  private native boolean nativeRunSearch(String s, String lang, double lat, double lon, int flags, int queryId);

  private native String getLastQuery();

  private native void clearLastQuery();

  private native void runInteractiveSearch(String query, String lang);
}
