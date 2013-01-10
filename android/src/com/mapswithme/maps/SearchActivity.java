package com.mapswithme.maps;

import java.util.Locale;

import android.app.ListActivity;
import android.content.Context;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.BaseAdapter;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.TextView;

import com.mapswithme.maps.location.LocationService;


public class SearchActivity extends ListActivity implements LocationService.Listener
{
  private static String TAG = "SearchActivity";

  private static class SearchAdapter extends BaseAdapter
  {
    private final SearchActivity m_context;
    private final LayoutInflater m_inflater;

    int m_count = 0;
    int m_resultID = 0;

    public SearchAdapter(SearchActivity context)
    {
      m_context = context;
      m_inflater = (LayoutInflater) m_context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
    }

    @Override
    public int getItemViewType(int position)
    {
      return 0;
    }

    @Override
    public int getViewTypeCount()
    {
      return 1;
    }

    @Override
    public int getCount()
    {
      return m_count;
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

      void initFromView(View v)
      {
        m_name = (TextView) v.findViewById(R.id.name);
        m_country = (TextView) v.findViewById(R.id.country);
        m_distance = (TextView) v.findViewById(R.id.distance);
        m_amenity = (TextView) v.findViewById(R.id.amenity);
        m_flag = (ArrowImage) v.findViewById(R.id.country_flag);
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

    @Override
    public View getView(int position, View convertView, ViewGroup parent)
    {
      ViewHolder holder = null;

      if (convertView == null)
      {
        holder = new ViewHolder();

        switch (getItemViewType(position))
        {
        case 0:
          convertView = m_inflater.inflate(R.layout.search_item, null);
          holder.initFromView(convertView);
          break;
        }

        convertView.setTag(holder);
      }
      else
      {
        holder = (ViewHolder) convertView.getTag();
      }

      //Log.d(TAG, "Getting result for result ID = " + m_resultID);

      final SearchResult r = m_context.getResult(position, m_resultID);
      if (r != null)
      {
        holder.m_name.setText(r.m_name);
        holder.m_country.setText(r.m_country);
        holder.m_amenity.setText(r.m_amenity);
        holder.m_distance.setText(r.m_distance);

        if (r.m_type == 1)
        {
          holder.m_flag.setVisibility(View.VISIBLE);

          if (r.m_flag != null && r.m_flag.length() > 0 && r.m_azimut < 0.0)
            holder.m_flag.setFlag(m_context.getResources(), r.m_flag);
          else
            holder.m_flag.setAzimut(r.m_azimut);

          // force invalidate arrow image
          holder.m_flag.invalidate();
        }
        else
          holder.m_flag.setVisibility(View.INVISIBLE);
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

    public void updateDistance()
    {
      notifyDataSetChanged();
    }

    /// Show tapped country or get suggestion.
    public String showCountry(int position)
    {
      final SearchResult r = m_context.getResult(position, m_resultID);
      if (r != null)
      {
        if (r.m_type == 1)
        {
          // show country and close activity
          SearchActivity.nativeShowItem(position);
          return null;
        }
        else
        {
          // advise suggestion
          return r.m_name;
        }
      }

      // return an empty string as a suggestion
      return "";
    }
  }

  private EditText getSearchBox()
  {
    return (EditText) findViewById(R.id.search_string);
  }

  private LinearLayout getSearchToolbar()
  {
    return (LinearLayout) findViewById(R.id.search_toolbar);
  }

  private LinearLayout getModeToolbar()
  {
    return (LinearLayout) findViewById(R.id.search_mode_toolbar);
  }

  private String getSearchString()
  {
    final String s = getSearchBox().getText().toString();
    Log.d(TAG, "Search string = " + s);
    return s;
  }

  private LocationService m_location;
  private ProgressBar m_progress;

  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);

    requestWindowFeature(Window.FEATURE_NO_TITLE);

    nativeInitSearch();

    m_location = ((MWMApplication) getApplication()).getLocationService();

    setContentView(R.layout.search_list_view);

    m_progress = (ProgressBar) findViewById(R.id.search_progress);

    final EditText v = getSearchBox();
    v.addTextChangedListener(new TextWatcher()
    {
      @Override
      public void afterTextChanged(Editable s)
      {
        runSearch();
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

    setListAdapter(new SearchAdapter(this));
  }

  @Override
  protected void onDestroy()
  {
    super.onDestroy();

    nativeFinishSearch();
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
    runSearch();
  }

  @Override
  protected void onPause()
  {
    super.onPause();

    m_location.stopUpdate(this);
  }

  private SearchAdapter getSA()
  {
    return (SearchAdapter) getListView().getAdapter();
  }

  @Override
  protected void onListItemClick(ListView l, View v, int position, long id)
  {
    super.onListItemClick(l, v, position, id);

    final String suggestion = getSA().showCountry(position);
    if (suggestion == null)
    {
      // close activity
      finish();
    }
    else
    {
      // set suggestion string and run search (this call invokes runSearch)
      runSearch(suggestion);
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
    getSA().updateDistance();
  }

  @Override
  public void onLocationUpdated(long time, double lat, double lon, float accuracy)
  {
    m_flags |= HAS_POSITION;

    m_lat = lat;
    m_lon = lon;

    if (!runSearch())
      updateDistance();
  }

  @Override
  public void onCompassUpdated(long time, double magneticNorth, double trueNorth, double accuracy)
  {
    @SuppressWarnings("deprecation")
    final int orientation = getWindowManager().getDefaultDisplay().getOrientation();
    final double correction = LocationService.getAngleCorrection(orientation);

    final double north = LocationService.correctAngle(trueNorth, correction);

    // if difference is more than 1 degree
    if (m_north == -1 || Math.abs(m_north - north) > 0.02)
    {
      m_north = north;
      //Log.d(TAG, "Compass updated, north = " + m_north);

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
        Log.d(TAG, "Show " + count + " results for id = " + resultID);

        // if this results for the last query - hide progress
        if (isCurrentResult(resultID))
          m_progress.setVisibility(View.GONE);

        // update list view content
        getSA().updateData(count, resultID);

        // scroll list view to the top
        setSelection(0);
      }
    });
  }

  /// @name Amenity buttons listeners.
  //@{
  public void onSearchFood(View v) { runSearch("food "); }
  public void onSearchMoney(View v) { runSearch("money "); }
  public void onSearchFuel(View v) { runSearch("fuel "); }
  public void onSearchShop(View v) { runSearch("shop "); }
  public void onSearchTransport(View v) { runSearch("transport "); }
  public void onSearchTourism(View v) { runSearch("tourism "); }
  //@}

  /// @name Search mode buttons listeners.
  //@{
  public void onSearchModeAll(View v) { runSearch(ALL); }
  public void onSearchModeNearMe(View v) { runSearch(AROUND_POSITION); }
  public void onSearchModeViewport(View v) { runSearch(IN_VIEWPORT); }
  //@}

  private void runSearch(String s)
  {
    final EditText box = getSearchBox();

    // this call invokes runSearch
    box.setText(s);

    // put cursor to the end of string
    box.setSelection(s.length());
  }

  /// @name These constants should be equal with search_params.hpp
  //@{
  private static final int AROUND_POSITION = 1;
  private static final int IN_VIEWPORT = 2;
  private static final int SEARCH_WORLD = 4;
  private static final int ALL = AROUND_POSITION | IN_VIEWPORT | SEARCH_WORLD;
  //@}
  private int m_searchMode = ALL;

  private void runSearch(int mode)
  {
    m_searchMode = mode;
    runSearch();
  }

  private boolean runSearch()
  {
    // TODO Need to get input language
    final String lang = Locale.getDefault().getLanguage();
    Log.d(TAG, "Current language = " + lang);

    final String s = getSearchString();

    final int id = m_queryID + QUERY_STEP;
    if (nativeRunSearch(s, lang, m_lat, m_lon, m_flags, m_searchMode, id))
    {
      // store current query
      m_queryID = id;
      //Log.d(TAG, "Current search query id =" + m_queryID);

      // mark that it's not the first query already
      m_flags |= NOT_FIRST_QUERY;

      // set toolbar visible only for empty search string
      final boolean emptyQuery = s.length() == 0;

      LinearLayout bar = getSearchToolbar();
      bar.setVisibility(emptyQuery ? View.VISIBLE : View.GONE);

      bar = getModeToolbar();
      bar.setVisibility(emptyQuery ? View.GONE : View.VISIBLE);

      // show search progress
      m_progress.setVisibility(View.VISIBLE);

      return true;
    }
    else
      return false;
  }

  public SearchAdapter.SearchResult getResult(int position, int queryID)
  {
    return nativeGetResult(position, queryID, m_lat, m_lon, (m_flags & HAS_POSITION) != 0, m_north);
  }

  private native void nativeInitSearch();
  private native void nativeFinishSearch();

  private static native SearchAdapter.SearchResult
  nativeGetResult(int position, int queryID, double lat, double lon, boolean mode, double north);

  private native boolean nativeRunSearch(String s, String lang,
                                         double lat, double lon, int flags,
                                         int searchMode, int queryID);
  private static native void nativeShowItem(int position);
}
