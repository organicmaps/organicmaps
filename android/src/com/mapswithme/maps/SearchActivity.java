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
import android.text.TextUtils;
import android.text.TextWatcher;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.BaseAdapter;
import android.widget.EditText;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.Spinner;
import android.widget.TextView;

import com.mapswithme.maps.base.MapsWithMeBaseListActivity;
import com.mapswithme.maps.location.LocationService;
import com.mapswithme.maps.search.SearchController;
import com.mapswithme.util.InputUtils;
import com.mapswithme.util.Language;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.statistics.Statistics;


public class SearchActivity extends MapsWithMeBaseListActivity implements LocationService.Listener
{
  private static String TAG = "SearchActivity";
  public static final String SEARCH_RESULT = "search_result";

  public static final String EXTRA_SCOPE = "search_scope";
  public static final String EXTRA_QUERY = "search_query";

  public static void startForSearch(Context context, String query, int scope)
  {
    final Intent i = new Intent(context, SearchActivity.class);
    i.putExtra(EXTRA_SCOPE, scope).putExtra(EXTRA_QUERY, query);
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
    private final SearchActivity m_context;
    private final LayoutInflater m_inflater;


    private final Resources m_resource;
    private final String m_packageName;

    private static final String m_categories[] = {
      "food",
      "shop",
      "hotel",
      "tourism",
      "entertainment",
      "atm",
      "bank",
      "transport",
      "fuel",
      "parking",
      "pharmacy",
      "hospital",
      "toilet",
      "post",
      "police"
    };

    private int m_count = -1;
    private int m_resultID = 0;

    public SearchAdapter(SearchActivity context)
    {
      m_context = context;
      m_inflater = (LayoutInflater) m_context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);

      m_resource = m_context.getResources();
      m_packageName = m_context.getApplicationContext().getPackageName();
    }

    private static final int CATEGORY_TYPE = 0;
    private static final int RESULT_TYPE = 1;
    private static final int MESSAGE_TYPE = 2;


    private boolean isShowCategories()
    {
      return m_context.isShowCategories();
    }

    private String getWarningForEmptyResults()
    {
      // First try to show warning if no country downloaded for viewport.
      if (m_context.m_searchMode != AROUND_POSITION)
      {
        final String name = m_context.getViewportCountryNameIfAbsent();
        if (name != null)
          return String.format(m_context.getString(R.string.download_viewport_country_to_search), name);
      }

      // If now position detected or no country downloaded for position.
      if (m_context.m_searchMode != IN_VIEWPORT)
      {
        final Location loc = m_context.m_location.getLastKnown();
        if (loc == null)
        {
          return m_context.getString(R.string.unknown_current_position);
        }
        else
        {
          final String name = m_context.getCountryNameIfAbsent(loc.getLatitude(), loc.getLongitude());
          if (name != null)
            return String.format(m_context.getString(R.string.download_location_country), name);
        }
      }

      return null;
    }

    @Override
    public boolean isEnabled(int position)
    {
      return (isShowCategories() || m_count > 0);
    }

    @Override
    public int getItemViewType(int position)
    {
      if (isShowCategories())
        return CATEGORY_TYPE;
      else
        return (m_count == 0 ? MESSAGE_TYPE : RESULT_TYPE);
    }

    @Override
    public int getViewTypeCount()
    {
      return 3;
    }

    @Override
    public int getCount()
    {
      if (isShowCategories())
        return m_categories.length;
      else if (m_count < 0)
        return 0;
      else
        return (m_count == 0 ? 1 : m_count + 1); // first additional item is "Show all result for"
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

    private static class ViewHolder
    {
      public TextView m_name = null;
      public TextView m_country = null;
      public TextView m_distance = null;
      public TextView m_amenity = null;
      public ArrowImage m_flag = null;

      void initFromView(View v, int type)
      {
        m_name = (TextView) v.findViewById(R.id.name);

        if (type != MESSAGE_TYPE)
          m_flag = (ArrowImage) v.findViewById(R.id.country_flag);

        if (type != CATEGORY_TYPE)
          m_country = (TextView) v.findViewById(R.id.country);

        if (type == RESULT_TYPE)
        {
          m_distance = (TextView) v.findViewById(R.id.distance);
          m_amenity = (TextView) v.findViewById(R.id.amenity);
        }
      }
    }

    /// Created from native code.
    public static class SearchResult
    {
      public String m_name;
      public String m_country;
      public String m_amenity;

      public String m_flag;
      public String m_distance;
      public double m_azimut;

      /// 0 - suggestion result
      /// 1 - feature result
      public int m_type;

      // Called from native code
      @SuppressWarnings("unused")
      public SearchResult(String suggestion)
      {
        m_name = suggestion;
        m_type = 0;
      }

      // Called from native code
      @SuppressWarnings("unused")
      public SearchResult(String name, String country, String amenity,
                          String flag, String distance, double azimut)
      {
        m_name = name;
        m_country = country;
        m_amenity = amenity;

        m_flag = flag;
        m_distance = distance;
        m_azimut = azimut;

        m_type = 1;
      }
    }

    private String getCategoryName(String strID)
    {
      final int id = m_resource.getIdentifier(strID, "string", m_packageName);
      if (id > 0)
      {
        return m_context.getString(id);
      }
      else
      {
        Log.e(TAG, "Failed to get resource id from: " + strID);
        return null;
      }
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent)
    {
      ViewHolder holder = null;

      if (convertView == null)
      {
        holder = new ViewHolder();

        switch (getItemViewType(position))
        {
        case CATEGORY_TYPE:
          convertView = m_inflater.inflate(R.layout.search_category_item, null);
          holder.initFromView(convertView, CATEGORY_TYPE);
          break;

        case RESULT_TYPE:
          convertView = m_inflater.inflate(R.layout.search_item, null);
          holder.initFromView(convertView, RESULT_TYPE);
          break;

        case MESSAGE_TYPE:
          convertView = m_inflater.inflate(R.layout.search_message_item, null);
          holder.initFromView(convertView, MESSAGE_TYPE);
          break;
        }

        convertView.setTag(holder);
      }
      else
      {
        holder = (ViewHolder) convertView.getTag();
      }

      if (isShowCategories())
      {
        // Show categories list.

        final String strID = m_categories[position];

        holder.m_name.setText(getCategoryName(strID));
        holder.m_flag.setFlag(m_resource, m_packageName, strID);
      }
      else if (m_count == 0)
      {
        // Show warning message.

        holder.m_name.setText(m_context.getString(R.string.no_search_results_found));

        final String msg = getWarningForEmptyResults();
        if (msg != null)
        {
          holder.m_country.setVisibility(View.VISIBLE);
          holder.m_country.setText(msg);
        }
        else
          holder.m_country.setVisibility(View.GONE);
      }
      else
      {
        // title item with "Show all" text
        if (position == 0)
        {
          holder.m_name.setText(m_context.getString(R.string.search_show_on_map));
          holder.m_country.setText(null);
          holder.m_amenity.setText(null);
          holder.m_distance.setText(null);
          holder.m_flag.clear();

          return convertView;
        }
        // 0 index is for multiple result
        // so real result are from 1
        --position;

        // Show search results.

        final SearchResult r = m_context.getResult(position, m_resultID);
        if (r != null)
        {
          holder.m_name.setText(r.m_name);
          holder.m_country.setText(r.m_country);
          holder.m_amenity.setText(r.m_amenity);
          holder.m_distance.setText(r.m_distance);

          if (r.m_type == 1)
          {
            if (r.m_flag != null && r.m_flag.length() > 0 && r.m_azimut < 0.0)
              holder.m_flag.setFlag(m_resource, m_packageName, r.m_flag);
            else
              holder.m_flag.setAzimut(r.m_azimut);
          }
          else
            holder.m_flag.clear();
        }
      }

      return convertView;
    }

    /// Update list data.
    public void updateData(int count, int resultID)
    {
      m_count = count;
      m_resultID = resultID;

      notifyDataSetChanged();
    }

    public void updateData()
    {
      notifyDataSetChanged();
    }

    public void updateCategories()
    {
      m_count = -1;
      updateData();
    }

    /// Show tapped country or get suggestion or get category to search.
    /// @return Suggestion string with space in the end (for full match purpose).
    public String onItemClick(int position)
    {
      if (isShowCategories())
      {

        final String category = getCategoryName(m_categories[position]);
        Statistics.INSTANCE.trackSearchCategoryClicked(m_context, category);

        return category + ' ';
      }
      else
      {
        // TODO handle all cases
        if (m_count > 0)
        {
          // Show all items was clicked
          if (position == 0)
          {
            SearchActivity.nativeShowAllSearchResults();
            return null;
          }

          // Specific result was clicked
          final int resIndex = position - 1;
          final SearchResult r = m_context.getResult(resIndex, m_resultID);
          if (r != null)
          {
            if (r.m_type == 1)
            {
              // show country and close activity
              SearchActivity.nativeShowItem(resIndex);
              return null;
            }
            else
            {
              // advise suggestion
              return r.m_name + ' ';
            }
          }
        }
      }


      // close activity in case of any error
      return null;
    }
  }

  private String getSearchString()
  {
    return mSearchBox.getText().toString();
  }

  private boolean isShowCategories()
  {
    return (getSearchString().length() == 0);
  }

  private LocationService m_location;
  // Search views
  private EditText mSearchBox;
  private ProgressBar mSearchProgress;
  private View mClearQueryBtn;
  private View mVoiceInput;
  private View mSearchIcon;
  //

  private Spinner m_modesSpinner;

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

    m_location = ((MWMApplication) getApplication()).getLocationService();

    setContentView(R.layout.search_list_view);
    setUpView();
    // Create search list view adapter.
    setListAdapter(new SearchAdapter(this));

    nativeConnect();

    //checking search intent
    final Intent intent = getIntent();
    if (intent != null && intent.hasExtra(EXTRA_QUERY))
    {
      mSearchBox.setText(intent.getStringExtra(EXTRA_QUERY));
      if (intent.hasExtra(EXTRA_SCOPE))
        m_modesSpinner.setSelection(intent.getIntExtra(EXTRA_SCOPE, 0));
      runSearch();
    }
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
    mClearQueryBtn.setOnClickListener(new OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        mSearchBox.getText().clear();
      }
    });

    // Initialize search edit box processor.
    mSearchBox = (EditText) findViewById(R.id.search_text_query);
    mSearchBox.addTextChangedListener(new TextWatcher()
    {
      @Override
      public void afterTextChanged(Editable s)
      {
        if (runSearch() == QUERY_EMPTY)
          showCategories();

        if (s.length() == 0) // enable voice input
        {
          UiUtils.invisible(mClearQueryBtn);
          UiUtils.hideIf(!InputUtils.isVoiceInputSupported(SearchActivity.this), mVoiceInput);
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

    // Initialize search modes spinner.
    m_modesSpinner = (Spinner) findViewById(R.id.search_modes_spinner);

    final ArrayAdapter<?> adapter = ArrayAdapter
                                    .createFromResource(this, R.array.search_modes, R.layout.simple_spinner_item);
    adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
    m_modesSpinner.setAdapter(adapter);

    // Default mode is AROUND_POSITION
    m_modesSpinner.setSelection(MWMApplication.get().nativeGetInt(SEARCH_MODE_SETTING, 1));

    m_modesSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener()
    {
      @Override
      public void onItemSelected(AdapterView<?> parent, View view, int position, long id)
      {
        int mode = ALL;
        switch (position)
        {
        case 1: mode = AROUND_POSITION; break;
        case 2: mode = IN_VIEWPORT; break;
        }

        // Save new search setting
        //
        // But track change before that
        final int oldSearchContext = getMwmApplication().nativeGetInt(SEARCH_MODE_SETTING, 1);
        final String[] contexts = {"All", "Around", "Viewport"};
        // value | val % 7 | real index in 'contexts'
        //     7 |       0 |    0
        //     1 |       1 |    1
        //     2 |       2 |    2
        final String from = contexts[oldSearchContext % 7];
        final String to   = contexts[mode % 7];

        if (!TextUtils.equals(from, to))
          Statistics.INSTANCE.trackSearchContextChanged(SearchActivity.this, from, to);
        // END of statistics

        getMwmApplication().nativeSetInt(SEARCH_MODE_SETTING, position);
        runSearch(mode);
      }

      @Override
      public void onNothingSelected(AdapterView<?> parent)
      {
      }
    });

    mVoiceInput.setOnClickListener(new OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        final Intent vrIntent = InputUtils.createIntentForVoiceRecognition(getResources().getString(R.string.search_map));
        startActivityForResult(vrIntent, RC_VOICE_RECOGNITIN);
      }
    });
  }

  @Override
  protected void onResume()
  {
    super.onResume();

    // Reset current mode flag - start first search.
    m_flags = 0;
    m_north = -1.0;

    m_location.startUpdate(this);

    // do the search immediately after resume
    Utils.setStringAndCursorToEnd(mSearchBox, getLastQuery());
    mSearchBox.requestFocus();
  }

  @Override
  protected void onPause()
  {
    m_location.stopUpdate(this);

    super.onPause();
  }

  private SearchAdapter getSA()
  {
    return (SearchAdapter) getListView().getAdapter();
  }

  @Override
  protected void onListItemClick(ListView l, View v, int position, long id)
  {
    super.onListItemClick(l, v, position, id);
    final String suggestion = getSA().onItemClick(position);
    if (suggestion == null)
    {
      presentResult(v, position);
    }
    else
    {
      // set suggestion string and run search (this call invokes runSearch)
      runSearch(suggestion);
    }
  }

  private void presentResult(View v, int position)
  {
    if (MWMApplication.get().isProVersion())
    {
      finish();
      // GetResult not always returns correct values?
      final SearchAdapter.ViewHolder vh = (SearchAdapter.ViewHolder) v.getTag();
      SearchController.get().setQuery(vh.m_name.getText().toString());

      MWMActivity.startWithSearchResult(this, position != 0);
    }
    else
    {
      // Should we add something more attractive?
      Utils.toastShortcut(this, R.string.search_available_in_pro_version);
    }
  }

  @Override
  public void onBackPressed()
  {
    if (!isShowCategories())
    {
      // invokes runSearch with empty string - adapter will show categories
      mSearchBox.setText("");
    }
    else
    {
      super.onBackPressed();
      SearchController.get().cancel();
    }
  }

  /// Current position.
  private double m_lat;
  private double m_lon;
  private double m_north = -1.0;

  /// @name These constants should be equal with
  /// Java_com_mapswithme_maps_SearchActivity_nativeRunSearch routine.
  //@{
  private static final int NOT_FIRST_QUERY = 1;
  private static final int HAS_POSITION = 2;
  //@}
  private int m_flags = 0;

  private void updateDistance()
  {
    getSA().updateData();
  }

  private void showCategories()
  {
    clearLastQuery();
    setSearchInProgress(false);
    getSA().updateCategories();
  }

  @Override
  public void onLocationUpdated(final Location l)
  {
    m_flags |= HAS_POSITION;

    m_lat = l.getLatitude();
    m_lon = l.getLongitude();

    if (runSearch() == SEARCH_SKIPPED)
      updateDistance();
  }


  private final static long COMPAS_DELTA = 300;
  private long mLastCompasUpdate;

  @Override
  public void onCompassUpdated(long time, double magneticNorth, double trueNorth, double accuracy)
  {
    if (isShowCategories())
      return;

    // We don't want to update view too often, it is slow
    // and breaks click listeners
    if (System.currentTimeMillis() - mLastCompasUpdate < COMPAS_DELTA)
      return;
    else
      mLastCompasUpdate = System.currentTimeMillis();

    final double north[] = { magneticNorth, trueNorth };
    m_location.correctCompassAngles(getWindowManager().getDefaultDisplay(), north);
    final double ret = (north[1] >= 0.0 ? north[1] : north[0]);

    // if difference is more than 1 degree
    if (m_north == -1 || Math.abs(m_north - ret) > 0.02)
    {
      m_north = ret;
      updateDistance();
    }
  }

  @Override
  public void onLocationError(int errorCode)
  {
  }

  private int m_queryID = 0;
  /// Make 5-step increment to leave space for middle queries.
  /// This constant should be equal with native SearchAdapter::QUERY_STEP;
  private final static int QUERY_STEP = 5;

  private boolean isCurrentResult(int id)
  {
    return (id >= m_queryID && id < m_queryID + QUERY_STEP);
  }

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

        if (!isShowCategories())
        {
          // update list view with results if we are not in categories mode
          getSA().updateData(count, resultID);

          // scroll list view to the top
          setSelection(0);
        }
      }
    });
  }

  private void runSearch(String s)
  {
    Utils.setStringAndCursorToEnd(mSearchBox, s);
  }

  /// @name These constants should be equal with search_params.hpp
  //@{
  private static final int AROUND_POSITION = 1;
  private static final int IN_VIEWPORT = 2;
  private static final int SEARCH_WORLD = 4;
  private static final int ALL = AROUND_POSITION | IN_VIEWPORT | SEARCH_WORLD;
  //@}
  private static final String SEARCH_MODE_SETTING = "SearchMode";
  private int m_searchMode = AROUND_POSITION;

  private void runSearch(int mode)
  {
    m_searchMode = mode;
    runSearch();
  }

  private static final int SEARCH_LAUNCHED = 0;
  private static final int QUERY_EMPTY = 1;
  private static final int SEARCH_SKIPPED = 2;

  private int runSearch()
  {
    final String s = getSearchString();
    if (s.length() == 0)
    {
      // do force search next time from categories list
      m_flags &= (~NOT_FIRST_QUERY);
      return QUERY_EMPTY;
    }


    final String lang = Language.getKeyboardInput(this);

    final int id = m_queryID + QUERY_STEP;
    if (nativeRunSearch(s, lang, m_lat, m_lon, m_flags, m_searchMode, id))
    {
      // store current query
      m_queryID = id;

      // mark that it's not the first query already - don't do force search
      m_flags |= NOT_FIRST_QUERY;

      setSearchInProgress(true);

      return SEARCH_LAUNCHED;
    }
    else
      return SEARCH_SKIPPED;
  }

  private void setSearchInProgress(boolean inProgress)
  {
    // TODO animate visibility change
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

  public SearchAdapter.SearchResult getResult(int position, int queryID)
  {
    return nativeGetResult(position, queryID, m_lat, m_lon, (m_flags & HAS_POSITION) != 0, m_north);
  }

  private native void nativeConnect();
  private native void nativeDisconnect();

  private static native SearchAdapter.SearchResult
  nativeGetResult(int position, int queryID, double lat, double lon, boolean mode, double north);

  private native boolean nativeRunSearch(String s, String lang,
                                         double lat, double lon, int flags,
                                         int searchMode, int queryID);

  private static native void nativeShowItem(int position);
  private static native void nativeShowAllSearchResults();

  private native String getCountryNameIfAbsent(double lat, double lon);
  private native String getViewportCountryNameIfAbsent();

  private native String getLastQuery();
  private native void clearLastQuery();


  // Handle voice recognition here
  private final static int RC_VOICE_RECOGNITIN = 0xCA11;

  @Override
  protected void onActivityResult(int requestCode, int resultCode, Intent data)
  {
    super.onActivityResult(requestCode, resultCode, data);

    if ((requestCode == RC_VOICE_RECOGNITIN) && (resultCode == Activity.RESULT_OK))
    {
      final String result = InputUtils.getMostConfidentResult(data);
      if (result != null)
        mSearchBox.setText(result);
    }
  }
}
