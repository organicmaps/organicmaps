package com.mapswithme.maps;

import android.annotation.SuppressLint;
import android.app.ActionBar;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;
import android.location.Location;
import android.os.Bundle;
import android.text.Editable;
import android.text.Html;
import android.text.Spanned;
import android.text.TextWatcher;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.view.inputmethod.EditorInfo;
import android.widget.AbsListView;
import android.widget.AbsListView.OnScrollListener;
import android.widget.BaseAdapter;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.TextView.OnEditorActionListener;

import com.mapswithme.maps.base.MapsWithMeBaseListActivity;
import com.mapswithme.maps.location.LocationService;
import com.mapswithme.maps.search.SearchController;
import com.mapswithme.util.InputUtils;
import com.mapswithme.util.Language;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.statistics.Statistics;


public class SearchActivity extends MapsWithMeBaseListActivity implements LocationService.LocationListener, OnClickListener
{
  public static final String EXTRA_QUERY = "search_query";
  /// @name These constants should be equal with
  /// Java_com_mapswithme_maps_SearchActivity_nativeRunSearch routine.
  //@{
  private static final int NOT_FIRST_QUERY = 1;
  private static final int HAS_POSITION = 2;
  /// Make 5-step increment to leave space for middle queries.
  /// This constant should be equal with native SearchAdapter::QUERY_STEP;
  private final static int QUERY_STEP = 5;
  /// @name These constants should be equal with search_params.hpp
  //@{
  private static final int AROUND_POSITION = 1;
  private static final int IN_VIEWPORT = 2;
  private static final int SEARCH_WORLD = 4;
  private static final int ALL = AROUND_POSITION | IN_VIEWPORT | SEARCH_WORLD;
  private int mSearchMode = ALL;
  //@}
  private static final int SEARCH_LAUNCHED = 0;
  private static final int QUERY_EMPTY = 1;
  private static final int SEARCH_SKIPPED = 2;
  // Handle voice recognition here
  private final static int RC_VOICE_RECOGNITION = 0xCA11;
  private static String TAG = "SearchActivity";
  // Search views
  private EditText mSearchEt;
  private ProgressBar mSearchProgress;
  private View mClearQueryBtn;
  private View mVoiceInput;
  private View mSearchIcon;
  /// Current position.
  private double mLat;
  private double mLon;
  private double mNorth = -1.0;
  //@}
  private int mFlags = 0;
  private int mQueryID = 0;

  public static void startForSearch(Context context, String query, int scope)
  {
    final Intent i = new Intent(context, SearchActivity.class);
    i.putExtra(EXTRA_QUERY, query);
    context.startActivity(i);
  }

  public static void startForSearch(Context context, String query)
  {
    final Intent i = new Intent(context, SearchActivity.class);
    i.putExtra(EXTRA_QUERY, query);
    context.startActivity(i);
  }

  private static class SearchAdapter extends BaseAdapter
  {
    private static final String mCategories[] = {
        "food",
        "hotel",
        "tourism",
        "transport",
        "fuel",
        "shop",
        "entertainment",
        "atm",
        "bank",
        "parking",
        "toilet",
        "pharmacy",
        "hospital",
        "post",
        "police",
        "wifi"
    };
    private static final int mIcons[] = {
        R.drawable.ic_food,
        R.drawable.ic_hotel,
        R.drawable.ic_tourism,
        R.drawable.ic_transport,
        R.drawable.ic_fuel,
        R.drawable.ic_shop,
        R.drawable.ic_entertainment,
        R.drawable.ic_atm,
        R.drawable.ic_bank,
        R.drawable.ic_parking,
        R.drawable.ic_toilet,
        R.drawable.ic_pharmacy,
        R.drawable.ic_hospital,
        R.drawable.ic_post,
        R.drawable.ic_police,
        R.drawable.ic_wifi
    };

    private static final int CATEGORY_TYPE = 0;
    private static final int RESULT_TYPE = 1;
    private static final int MESSAGE_TYPE = 2;
    private static final int VIEW_TYPE_COUNT = 3;
    private final SearchActivity mContext;
    private final LayoutInflater mInflater;
    private final Resources mResources;
    private final String mPackageName;
    private int mCount = -1;
    private int mResultID = 0;

    public SearchAdapter(SearchActivity context)
    {
      mContext = context;
      mInflater = (LayoutInflater) mContext.getSystemService(Context.LAYOUT_INFLATER_SERVICE);

      mResources = mContext.getResources();
      mPackageName = mContext.getApplicationContext().getPackageName();
    }

    private boolean doShowCategories()
    {
      return mContext.doShowCategories();
    }

    private String getWarningForEmptyResults()
    {
      // First try to show warning if no country downloaded for viewport.
      if (mContext.mSearchMode != AROUND_POSITION)
      {
        final String name = Framework.nativeGetViewportCountryNameIfAbsent();
        if (name != null)
          return String.format(mContext.getString(R.string.download_viewport_country_to_search), name);
      }

      // If now position detected or no country downloaded for position.
      if (mContext.mSearchMode != IN_VIEWPORT)
      {
        final Location loc = LocationService.INSTANCE.getLastLocation();
        if (loc == null)
        {
          return mContext.getString(R.string.unknown_current_position);
        }
        else
        {
          final String name = Framework.nativeGetCountryNameIfAbsent(loc.getLatitude(), loc.getLongitude());
          if (name != null)
            return String.format(mContext.getString(R.string.download_location_country), name);
        }
      }

      return null;
    }

    @Override
    public boolean isEnabled(int position)
    {
      return (doShowCategories() || getCount() > 0);
    }

    @Override
    public int getItemViewType(int position)
    {
      if (doShowCategories())
        return CATEGORY_TYPE;
      else if (position == 0 && doShowSearchOnMapButton())
        return MESSAGE_TYPE;
      else
        return RESULT_TYPE;
    }

    @Override
    public int getViewTypeCount()
    {
      return VIEW_TYPE_COUNT;
    }

    @Override
    public int getCount()
    {
      if (doShowCategories())
        return mCategories.length;
      else if (mCount < 0)
        return 0;
      else if (doShowSearchOnMapButton())
        return mCount + 1;
      else
        return mCount;
    }

    private boolean doShowSearchOnMapButton()
    {
      if (mCount == 0)
        return true;

      final SearchResult result = mContext.getResult(0, mResultID);
      return result != null && result.mType != SearchResult.TYPE_SUGGESTION;
    }

    public int getPositionInResults(int position)
    {
      if (doShowSearchOnMapButton())
        return position - 1;
      else
        return position;
    }


    @Override
    public Object getItem(int position)
    {
      return position;
    }

    @Override
    public long getItemId(int position)
    {
      return position;
    }

    private String getCategoryName(String strID)
    {
      final int id = mResources.getIdentifier(strID, "string", mPackageName);
      if (id > 0)
      {
        return mContext.getString(id);
      }
      else
      {
        Log.e(TAG, "Failed to getInstance resource id from: " + strID);
        return null;
      }
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent)
    {
      ViewHolder holder;
      final int viewType = getItemViewType(position);
      if (convertView == null)
      {
        switch (viewType)
        {
        case CATEGORY_TYPE:
          convertView = mInflater.inflate(R.layout.search_category_item, parent, false);
          break;
        case RESULT_TYPE:
          convertView = mInflater.inflate(R.layout.search_item, parent, false);
          break;
        case MESSAGE_TYPE:
          convertView = mInflater.inflate(R.layout.search_message_item, parent, false);
          break;
        }
        holder = new ViewHolder(convertView, viewType);

        convertView.setTag(holder);
      }
      else
        holder = (ViewHolder) convertView.getTag();

      switch (viewType)
      {
      case CATEGORY_TYPE:
        bindCategoryView(holder, position);
        break;
      case RESULT_TYPE:
        bindResultView(holder, getPositionInResults(position));
        break;
      case MESSAGE_TYPE:
        bindMessageView(holder, position);
        break;
      }

      return convertView;
    }

    private void bindResultView(ViewHolder holder, int position)
    {
      final SearchResult r = mContext.getResult(position, mResultID);
      if (r != null)
      {
        String country = null;
        String dist = null;
        Spanned s;
        if (r.mType == SearchResult.TYPE_FEATURE)
        {
          if (r.mHighlightRanges.length > 0)
          {
            StringBuilder builder = new StringBuilder();
            int pos = 0, j = 0, n = r.mHighlightRanges.length / 2;

            for (int i = 0; i < n; ++i)
            {
              int start = r.mHighlightRanges[j++];
              int len = r.mHighlightRanges[j++];

              builder.append(r.mName.substring(pos, start));
              builder.append("<font color=\"green\">");
              builder.append(r.mName.substring(start, start + len));
              builder.append("</font>");

              pos = start + len;
            }
            builder.append(r.mName.substring(pos));
            s = Html.fromHtml(builder.toString());
          }
          else
            s = Html.fromHtml(r.mName);

          country = r.mCountry;
          dist = r.mDistance;
          UiUtils.hide(holder.mImageLeft);
          holder.mView.setBackgroundResource(R.drawable.bg_toolbar_button_selector);
        }
        else
        {
          s = Html.fromHtml(r.mName);

          holder.mView.setBackgroundResource(R.drawable.bg_search_item_green_selector);
          UiUtils.show(holder.mImageLeft);
          holder.mImageLeft.setImageResource(R.drawable.ic_search);
        }

        UiUtils.setTextAndShow(holder.mName, s);
        UiUtils.setTextAndHideIfEmpty(holder.mCountry, country);
        UiUtils.setTextAndHideIfEmpty(holder.mItemType, r.mAmenity);
        UiUtils.setTextAndHideIfEmpty(holder.mDistance, dist);
      }
    }

    private void bindCategoryView(ViewHolder holder, int position)
    {
      UiUtils.setTextAndShow(holder.mName, getCategoryName(mCategories[position]));
      holder.mImageLeft.setImageResource(mIcons[position]);
    }

    private void bindMessageView(ViewHolder holder, int position)
    {
      UiUtils.setTextAndShow(holder.mName, mContext.getString(R.string.search_on_map));
    }

    /// Update list data.
    public void updateData(int count, int resultID)
    {
      mCount = count;
      mResultID = resultID;

      notifyDataSetChanged();
    }

    public void updateData()
    {
      notifyDataSetChanged();
    }

    public void updateCategories()
    {
      mCount = -1;
      updateData();
    }

    /// Show tapped country or getInstance suggestion or getInstance category to search.
    /// @return Suggestion string with space in the end (for full match purpose).
    public String onItemClick(int position)
    {
      switch (getItemViewType(position))
      {
      case MESSAGE_TYPE:
        Statistics.INSTANCE.trackSimpleNamedEvent(Statistics.EventName.SEARCH_ON_MAP_CLICKED);
        return null;
      case RESULT_TYPE:
        final int resIndex = getPositionInResults(position);
        final SearchResult r = mContext.getResult(resIndex, mResultID);
        if (r != null)
        {
          if (r.mType == SearchResult.TYPE_FEATURE)
          {
            // show country and close activity
            SearchActivity.nativeShowItem(resIndex);
            return null;
          }
          else
          {
            // advise suggestion
            return r.mSuggestion;
          }
        }
        break;
      case CATEGORY_TYPE:
        final String category = getCategoryName(mCategories[position]);
        Statistics.INSTANCE.trackSearchCategoryClicked(category);

        return category + ' ';
      }

      return null;
    }

    private static class ViewHolder
    {
      public View mView;
      public TextView mName;
      public TextView mCountry;
      public TextView mDistance;
      public TextView mItemType;
      public ImageView mImageLeft;

      public ViewHolder(View v, int type)
      {
        mView = v;
        mName = (TextView) v.findViewById(R.id.tv_search_item_title);

        switch (type)
        {
        case CATEGORY_TYPE:
          mImageLeft = (ImageView) v.findViewById(R.id.iv_search_category);
          break;
        case RESULT_TYPE:
          mImageLeft = (ImageView) v.findViewById(R.id.iv_search_image);
          mDistance = (TextView) v.findViewById(R.id.tv_search_distance);
          mCountry = (TextView) v.findViewById(R.id.tv_search_item_subtitle);
          mItemType = (TextView) v.findViewById(R.id.tv_search_item_type);
          break;
        case MESSAGE_TYPE:
          mImageLeft = (ImageView) v.findViewById(R.id.iv_search_image);
          mCountry = (TextView) v.findViewById(R.id.tv_search_item_subtitle);
          break;
        }
      }
    }

    /// Created from native code.
    public static class SearchResult
    {
      public String mName;
      public String mSuggestion;
      public String mCountry;
      public String mAmenity;
      public String mDistance;

      /// 0 - suggestion result
      /// 1 - feature result
      public static final int TYPE_SUGGESTION = 0;
      public static final int TYPE_FEATURE = 1;

      public int mType;
      public int[] mHighlightRanges;


      // Called from native code
      @SuppressWarnings("unused")
      public SearchResult(String name, String suggestion, int[] highlightRanges)
      {
        mName = name;
        mSuggestion = suggestion;
        mType = TYPE_SUGGESTION;

        mHighlightRanges = highlightRanges;
      }

      // Called from native code
      @SuppressWarnings("unused")
      public SearchResult(String name, String country, String amenity,
                          String distance, int[] highlightRanges)
      {
        mName = name;
        mCountry = country;
        mAmenity = amenity;
        mDistance = distance;

        mType = TYPE_FEATURE;

        mHighlightRanges = highlightRanges;
      }

    }
  }

  private static native SearchAdapter.SearchResult
  nativeGetResult(int position, int queryID, double lat, double lon, boolean mode, double north);

  private static native void nativeShowItem(int position);

  private static native void nativeShowAllSearchResults();

  private String getSearchString()
  {
    return mSearchEt.getText().toString();
  }

  private boolean doShowCategories()
  {
    return getSearchString().length() == 0;
  }

  //private final static long COMPAS_DELTA = 300;
  //private long mLastCompasUpdate;

  @SuppressLint("NewApi")
  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);

    if (Utils.apiEqualOrGreaterThan(11))
    {
      final ActionBar actionBar = getActionBar();
      if (actionBar != null)
        actionBar.hide();
    }

    setContentView(R.layout.search_list_view);
    setUpView();
    // Create search list view adapter.
    setListAdapter(new SearchAdapter(this));

    nativeConnect();

    //checking search intent
    final Intent intent = getIntent();
    if (intent != null && intent.hasExtra(EXTRA_QUERY))
    {
      mSearchEt.setText(intent.getStringExtra(EXTRA_QUERY));
      runSearch();
    }

    mSearchEt.setOnEditorActionListener(new OnEditorActionListener()
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
            presentResult(0);
          return true;
        }
        return false;
      }
    });
  }

  @Override
  protected void onDestroy()
  {
    nativeDisconnect();
    super.onDestroy();
  }

  private void setUpView()
  {
    mVoiceInput = findViewById(R.id.search_voice_input);
    mSearchIcon = findViewById(R.id.search_icon);
    mSearchProgress = (ProgressBar) findViewById(R.id.search_progress);
    mClearQueryBtn = findViewById(R.id.search_image_clear);
    mClearQueryBtn.setOnClickListener(this);

    // Initialize search edit box processor.
    mSearchEt = (EditText) findViewById(R.id.search_text_query);
    mSearchEt.addTextChangedListener(new TextWatcher()
    {
      @Override
      public void afterTextChanged(Editable s)
      {
        if (runSearch() == QUERY_EMPTY)
          showCategories();

        if (s.length() == 0) // enable voice input
        {
          UiUtils.invisible(mClearQueryBtn);
          UiUtils.showIf(InputUtils.isVoiceInputSupported(SearchActivity.this), mVoiceInput);
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

    findViewById(R.id.btn_cancel_search).setOnClickListener(this);
  }

  @Override
  protected void onResume()
  {
    super.onResume();

    // Reset current mode flag - start first search.
    mFlags = 0;
    mNorth = -1.0;

    LocationService.INSTANCE.startUpdate(this);

    // do the search immediately after resume
    Utils.setTextAndCursorToEnd(mSearchEt, getLastQuery());
    mSearchEt.requestFocus();
  }

  @Override
  protected void onPause()
  {
    LocationService.INSTANCE.stopUpdate(this);

    super.onPause();
  }

  private SearchAdapter getSearchAdapter()
  {
    return (SearchAdapter) getListView().getAdapter();
  }

  @Override
  protected void onListItemClick(ListView l, View v, int position, long id)
  {
    super.onListItemClick(l, v, position, id);
    final String suggestion = getSearchAdapter().onItemClick(position);
    if (suggestion == null)
    {
      presentResult(position);
    }
    else
    {
      // set suggestion string and run search (this call invokes runSearch)
      runSearch(suggestion);
    }
  }

  private void presentResult(int position)
  {
    // If user searched for something, then clear API layer
    SearchController.getInstance().cancelApiCall();

    if (BuildConfig.IS_PRO)
    {
      finish();

      // Put query string for "View on map" or feature name for search result.
      final boolean allResults = (position == 0);
      final String query = getSearchString();

      SearchController.getInstance().setQuery(allResults ? query : "");

      if (allResults)
      {
        SearchActivity.nativeShowAllSearchResults();
        runInteractiveSearch(query, Language.getKeyboardInput(this));
      }

      MWMActivity.startWithSearchResult(this, !allResults);
    }
    else
    {
      /// @todo Should we add something more attractive?
      Utils.toastShortcut(this, R.string.search_available_in_pro_version);
    }
  }

  @Override
  public void onBackPressed()
  {
    super.onBackPressed();
    SearchController.getInstance().cancel();
  }

  private void updateDistance()
  {
    getSearchAdapter().updateData();
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
      updateDistance();
  }

  @Override
  public void onCompassUpdated(long time, double magneticNorth, double trueNorth, double accuracy)
  {
  }

  @Override
  public void onDrivingHeadingUpdated(long time, double heading)
  {
  }

  @Override
  public void onLocationError(int errorCode)
  {
  }

  private boolean isCurrentResult(int id)
  {
    return (id >= mQueryID && id < mQueryID + QUERY_STEP);
  }

  // Called from native code
  @SuppressWarnings("unused")
  public void updateData(final int count, final int resultID)
  {
    runOnUiThread(new Runnable()
    {
      @Override
      public void run()
      {
        // if this results for the last query - hide progress
        if (isCurrentResult(resultID))
          setSearchInProgress(false);

        if (!doShowCategories())
        {
          // update list view with results if we are not in categories mode
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
      // do force search next time from categories list
      mFlags &= (~NOT_FIRST_QUERY);
      return QUERY_EMPTY;
    }

    final int id = mQueryID + QUERY_STEP;
    if (nativeRunSearch(s, Language.getKeyboardInput(this), mLat, mLon, mFlags, mSearchMode, id))
    {
      // store current query
      mQueryID = id;

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
    {
      UiUtils.show(mSearchProgress);
      UiUtils.invisible(mSearchIcon);
    }
    else // search is completed
    {
      UiUtils.invisible(mSearchProgress);
      UiUtils.show(mSearchIcon);
    }
  }

  @Override
  public void onClick(View v)
  {
    switch (v.getId())
    {
    case R.id.btn_cancel_search:
      SearchController.getInstance().setQuery("");
      SearchController.getInstance().cancel();
      clearLastQuery();
      finish();
      break;
    case R.id.search_image_clear:
      mSearchEt.getText().clear();
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

  private native void nativeConnect();

  private native void nativeDisconnect();

  private native boolean nativeRunSearch(String s, String lang,
                                         double lat, double lon, int flags,
                                         int searchMode, int queryID);

  private native String getLastQuery();

  private native void clearLastQuery();

  private native void runInteractiveSearch(String query, String lang);

  @Override
  protected void onActivityResult(int requestCode, int resultCode, Intent data)
  {
    super.onActivityResult(requestCode, resultCode, data);

    if ((requestCode == RC_VOICE_RECOGNITION) && (resultCode == Activity.RESULT_OK))
    {
      final String result = InputUtils.getMostConfidentResult(data);
      if (result != null)
        mSearchEt.setText(result);
    }
  }
}
