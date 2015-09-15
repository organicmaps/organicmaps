package com.mapswithme.maps.search;

import android.app.Activity;
import android.content.Intent;
import android.location.Location;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentTransaction;
import android.support.v7.widget.RecyclerView;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.EditorInfo;
import android.widget.EditText;
import android.widget.ProgressBar;
import android.widget.TextView;

import com.mapswithme.country.ActiveCountryTree;
import com.mapswithme.country.CountrySuggestFragment;
import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MwmActivity;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmRecyclerFragment;
import com.mapswithme.maps.base.OnBackPressListener;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.sound.TtsPlayer;
import com.mapswithme.util.InputUtils;
import com.mapswithme.util.Language;
import com.mapswithme.util.StringUtils;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.statistics.Statistics;


public class SearchFragment extends BaseMwmRecyclerFragment implements View.OnClickListener, LocationHelper.LocationListener, OnBackPressListener, NativeSearchListener
{
  // These constants should be equal with Java_com_mapswithme_maps_SearchFragment_nativeRunSearch routine.
  private static final int NOT_FIRST_QUERY = 1;
  private static final int HAS_POSITION = 2;
  // Make 5-step increment to leave space for middle queries.
  // This constant should be equal with native SearchAdapter::QUERY_STEP;
  private static final int QUERY_STEP = 5;
  private static final int STATUS_SEARCH_LAUNCHED = 0;
  private static final int STATUS_QUERY_EMPTY = 1;
  private static final int STATUS_SEARCH_SKIPPED = 2;
  private static final int RC_VOICE_RECOGNITION = 0xCA11;
  private static final double DUMMY_NORTH = -1;
  // Search views
  private EditText mEtSearchQuery;
  private ProgressBar mPbSearch;
  private View mBtnClearQuery;
  private View mBtnVoice;
  // Current position.
  private double mLat;
  private double mLon;

  private int mSearchFlags;
  private int mQueryId;
  private SearchAdapter mSearchAdapter;
  private CategoriesAdapter mCategoriesAdapter;

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
    initView(root);
    initAdapters();
    refreshContent();
    nativeConnectSearchListener();
    readArguments(getArguments());
    if (getToolbar() != null)
      getToolbar().setNavigationOnClickListener(new View.OnClickListener()
      {
        @Override
        public void onClick(View v)
        {
          if (getSearchQuery().isEmpty())
          {
            navigateUpToParent();
            return;
          }

          setSearchQuery("");
          SearchToolbarController.cancelSearch();
          refreshContent();
        }
      });
  }

  @Override
  public void onResume()
  {
    super.onResume();
    LocationHelper.INSTANCE.addLocationListener(this);
    setSearchQuery(nativeGetLastQuery());
    mEtSearchQuery.requestFocus();
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
    nativeDisconnectSearchListener();
    super.onDestroy();
  }

  protected String getSearchQuery()
  {
    return mEtSearchQuery.getText().toString();
  }

  protected void setSearchQuery(String query)
  {
    Utils.setTextAndCursorToEnd(mEtSearchQuery, query);
  }

  private void initView(View root)
  {
    mBtnVoice = root.findViewById(R.id.search_voice_input);
    mBtnVoice.setOnClickListener(this);
    mPbSearch = (ProgressBar) root.findViewById(R.id.search_progress);
    mBtnClearQuery = root.findViewById(R.id.search_image_clear);
    mBtnClearQuery.setOnClickListener(this);

    mEtSearchQuery = (EditText) root.findViewById(R.id.search_text_query);
    mEtSearchQuery.addTextChangedListener(new StringUtils.SimpleTextWatcher()
    {
      @Override
      public void onTextChanged(CharSequence s, int start, int before, int count)
      {
        if (!isAdded())
          return;

        // TODO: This code only for demonstration purposes and will be removed soon
        if (tryChangeMapStyle(s.toString()))
          return;

        if (runSearch() == STATUS_QUERY_EMPTY)
          showCategories();

        // TODO: This code only for demonstration purposes and will be removed soon
        if (trySwitchOnTurnSound(s.toString()))
          return;

        if (s.length() == 0)
        {
          UiUtils.hide(mBtnClearQuery);
          UiUtils.showIf(InputUtils.isVoiceInputSupported(getActivity()), mBtnVoice);
        }
        else
        {
          UiUtils.show(mBtnClearQuery);
          UiUtils.hide(mBtnVoice);
        }
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

        if ((isSearchDown || isActionSearch) && mSearchAdapter.getItemCount() > 0)
        {
          Statistics.INSTANCE.trackSimpleNamedEvent(Statistics.EventName.SEARCH_KEY_CLICKED);
          if (!doShowCategories())
            showAllResultsOnMap();
          return true;
        }
        return false;
      }
    });

    getRecyclerView().addOnScrollListener(new RecyclerView.OnScrollListener()
    {
      @Override
      public void onScrollStateChanged(RecyclerView recyclerView, int newState)
      {
        if (newState != RecyclerView.SCROLL_STATE_DRAGGING)
          return;
        InputUtils.hideKeyboard(mEtSearchQuery);
        InputUtils.removeFocusEditTextHack(mEtSearchQuery);
      }
    });
  }

  /**
   * If search string is empty - show search categories.
   */
  protected boolean doShowCategories()
  {
    return getSearchQuery().isEmpty();
  }

  private void initAdapters()
  {
    mSearchAdapter = new SearchAdapter(this);
    mCategoriesAdapter = new CategoriesAdapter(this);
  }

  private void refreshContent()
  {
    final RecyclerView recyclerView = getRecyclerView();

    if (!getSearchQuery().isEmpty())
    {
      hideDownloadSuggest();
      recyclerView.setAdapter(mSearchAdapter);
    }
    else if (doShowDownloadSuggest())
      showDownloadSuggest();
    else
    {
      hideDownloadSuggest();
      recyclerView.setAdapter(mCategoriesAdapter);
    }
  }

  private boolean doShowDownloadSuggest()
  {
    return ActiveCountryTree.getTotalDownloadedCount() == 0;
  }

  private void showDownloadSuggest()
  {
    getRecyclerView().setVisibility(View.GONE);
    final FragmentManager manager = getChildFragmentManager();
    final String fragmentName = CountrySuggestFragment.class.getName();
    Fragment fragment = manager.findFragmentByTag(fragmentName);
    if (fragment == null || fragment.isDetached() || fragment.isRemoving())
    {
      final FragmentTransaction transaction = manager.beginTransaction();
      fragment = Fragment.instantiate(getActivity(), fragmentName, null);
      transaction.add(R.id.inner_fragment_container, fragment, fragmentName).commit();
    }
  }

  private void hideDownloadSuggest()
  {
    getRecyclerView().setVisibility(View.VISIBLE);
    final FragmentManager manager = getChildFragmentManager();
    final Fragment fragment = manager.findFragmentByTag(CountrySuggestFragment.class.getName());
    if (fragment != null && !fragment.isRemoving() && !fragment.isDetached())
    {
      final FragmentTransaction transaction = manager.beginTransaction();
      transaction.remove(fragment).commit();
    }
  }

  private void readArguments(Bundle arguments)
  {
    if (arguments == null)
      return;

    final String query = arguments.getString(SearchActivity.EXTRA_QUERY);
    if (query != null)
      setSearchQuery(query);
  }

  private void hideSearch()
  {
    mEtSearchQuery.setText(null);
    InputUtils.hideKeyboard(mEtSearchQuery);
    navigateUpToParent();
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
    MwmActivity.setMapStyle(getActivity(), mapStyle);

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
    SearchToolbarController.cancelApiCall();
    SearchToolbarController.setQuery("");
    nativeShowItem(resultIndex);
    InputUtils.hideKeyboard(mEtSearchQuery);
    navigateUpToParent();
  }

  protected void showAllResultsOnMap()
  {
    nativeShowAllSearchResults();
    runInteractiveSearch(getSearchQuery(), Language.getKeyboardLocale());
    SearchToolbarController.setQuery(getSearchQuery());
    navigateUpToParent();
  }

  private void showCategories()
  {
    nativeClearLastQuery();
    displaySearchProgress(false);
    refreshContent();
  }

  @Override
  public void onLocationUpdated(final Location l)
  {
    mSearchFlags |= HAS_POSITION;

    mLat = l.getLatitude();
    mLon = l.getLongitude();

    if (runSearch() == STATUS_SEARCH_SKIPPED)
      mSearchAdapter.notifyDataSetChanged();
  }

  @Override
  public void onCompassUpdated(long time, double magneticNorth, double trueNorth, double accuracy) {}

  @Override
  public void onLocationError(int errorCode) {}

  private int runSearch()
  {
    final String query = getSearchQuery();
    if (query.isEmpty())
    {
      // do force search next time from empty list
      mSearchFlags &= (~NOT_FIRST_QUERY);
      return STATUS_QUERY_EMPTY;
    }

    final int id = mQueryId + QUERY_STEP;
    if (nativeRunSearch(query, Language.getKeyboardLocale(), mLat, mLon, mSearchFlags, id))
    {
      mQueryId = id;
      // mark that it's not the first query already - don't do force search
      mSearchFlags |= NOT_FIRST_QUERY;
      displaySearchProgress(true);
      mSearchAdapter.notifyDataSetChanged();
      return STATUS_SEARCH_LAUNCHED;
    }
    else
      return STATUS_SEARCH_SKIPPED;
  }

  @Override
  public void onResultsUpdate(final int count, final int resultId)
  {
    getActivity().runOnUiThread(new Runnable()
    {
      @Override
      public void run()
      {
        if (!isAdded())
          return;

        if (doShowCategories())
          return;

        refreshContent();
        mSearchAdapter.refreshData(count, resultId);
        getRecyclerView().scrollToPosition(0);
      }
    });
  }

  @Override
  public void onResultsEnd()
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

  private void displaySearchProgress(boolean inProgress)
  {
    if (inProgress)
      UiUtils.show(mPbSearch);
    else
      UiUtils.invisible(mPbSearch);
  }

  @Override
  public void onClick(View v)
  {
    switch (v.getId())
    {
    case R.id.search_image_clear:
      mEtSearchQuery.setText(null);
      SearchToolbarController.cancelApiCall();
      SearchToolbarController.cancelSearch();
      break;
    case R.id.search_voice_input:
      final Intent vrIntent = InputUtils.createIntentForVoiceRecognition(getResources().getString(R.string.search_map));
      startActivityForResult(vrIntent, RC_VOICE_RECOGNITION);
      break;
    }
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

  @Override
  public boolean onBackPressed()
  {
    return false;
  }

  public SearchResult getResult(int position, int queryId)
  {
    return nativeGetResult(position, queryId, mLat, mLon, (mSearchFlags & HAS_POSITION) != 0, DUMMY_NORTH);
  }

  public static native void nativeShowItem(int position);

  public static native void nativeShowAllSearchResults();

  private static native SearchResult nativeGetResult(int position, int queryId,
                                                     double lat, double lon, boolean mode, double north);

  private native void nativeConnectSearchListener();

  private native void nativeDisconnectSearchListener();

  private native boolean nativeRunSearch(String s, String lang, double lat, double lon, int flags, int queryId);

  private native String nativeGetLastQuery();

  private native void nativeClearLastQuery();

  private native void runInteractiveSearch(String query, String lang);
}
