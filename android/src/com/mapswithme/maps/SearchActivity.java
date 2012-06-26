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
import android.widget.BaseAdapter;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.TextView;

import com.mapswithme.maps.location.LocationService;


public class SearchActivity extends ListActivity implements LocationService.Listener
{
  private static String TAG = "SearchActivity";

  private static class SearchAdapter extends BaseAdapter
  {
    private SearchActivity m_context;
    private LayoutInflater m_inflater;

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

      public SearchResult(String suggestion)
      {
        m_name = suggestion;

        m_type = 0;
      }
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

          if (r.m_flag.length() > 0 && r.m_azimut < 0.0)
            holder.m_flag.setFlag(m_context.getResources(), r.m_flag);
          else
            holder.m_flag.setAzimut(r.m_azimut);
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

  private String getSearchString()
  {
    final String s = getSearchBox().getText().toString();
    Log.d(TAG, "Search string = " + s);
    return s;
  }

  private LocationService m_location;

  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);

    nativeInitSearch();

    m_location = ((MWMApplication) getApplication()).getLocationService();

    setContentView(R.layout.search_list_view);

    EditText v = getSearchBox();
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
    m_mode = 0;
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

  /// It's should be equal to search::SearchParams::ModeT
  /// Possible values:\n
  /// m_mode % 2 == 0 - first search query;\n
  /// m_mode >= 2 - position exists;\n
  int m_mode = 0;

  @Override
  public void onLocationUpdated(long time, double lat, double lon, float accuracy)
  {
    if (m_mode < 2)
      m_mode += 2;

    m_lat = lat;
    m_lon = lon;

    runSearch();
  }

  @Override
  public void onCompassUpdated(long time, double magneticNorth, double trueNorth, double accuracy)
  {
    final int orientation = getWindowManager().getDefaultDisplay().getOrientation();
    final double correction = LocationService.getAngleCorrection(orientation);

    m_north = LocationService.correctAngle(trueNorth, correction);

    getSA().updateDistance();
  }

  @Override
  public void onLocationStatusChanged(int status)
  {
  }

  private int m_queryID = 0;

  /// Make 5-step increment to leave space for middle queries.
  /// This constant should be equal with native SearchAdapter::QUERY_STEP;
  private final static int QUERY_STEP = 5;

  public void updateData(final int count, final int resultID)
  {
    runOnUiThread(new Runnable()
    {
      @Override
      public void run()
      {
        Log.d(TAG, "Show " + count + " results for id = " + resultID);

        // update list view content
        getSA().updateData(count, resultID);

        // scroll list view to the top
        setSelection(0);
      }
    });
  }

  /// @name Amenity buttons listeners
  //@{
  public void onSearchFood(View v) { runSearch("food "); }
  public void onSearchMoney(View v) { runSearch("money "); }
  public void onSearchFuel(View v) { runSearch("fuel "); }
  public void onSearchShop(View v) { runSearch("shop "); }
  public void onSearchTransport(View v) { runSearch("transport "); }
  public void onSearchTourism(View v) { runSearch("tourism "); }
  //@}

  private void runSearch(String s)
  {
    EditText box = getSearchBox();

    // this call invokes runSearch
    box.setText(s);

    // put cursor to the end of string
    box.setSelection(s.length());
  }

  private void runSearch()
  {
    // TODO Need to get input language
    final String lang = Locale.getDefault().getLanguage();
    Log.d(TAG, "Current language = " + lang);

    final String s = getSearchString();

    m_queryID += QUERY_STEP;
    if (nativeRunSearch(s, lang, m_lat, m_lon, m_mode, m_queryID))
    {
      // mark that it's not the first query already
      if (m_mode % 2 == 0) ++m_mode;

      // set toolbar visible only for empty search string
      LinearLayout bar = getSearchToolbar();
      bar.setVisibility(s.length() == 0 ? View.VISIBLE : View.GONE);
    }
  }

  public SearchAdapter.SearchResult getResult(int position, int queryID)
  {
    return nativeGetResult(position, queryID, m_lat, m_lon, m_mode, m_north);
  }

  private native void nativeInitSearch();
  private native void nativeFinishSearch();

  private static native SearchAdapter.SearchResult
  nativeGetResult(int position, int queryID, double lat, double lon, int mode, double north);

  private native boolean nativeRunSearch(String s, String lang,
                                         double lat, double lon, int mode, int queryID);
  private static native void nativeShowItem(int position);
}
