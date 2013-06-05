
package com.mapswithme.capitals;

import android.app.ListActivity;
import android.content.Context;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.TextView;

import com.mapswithme.maps.api.MWMPoint;
import com.mapswithme.maps.api.MapsWithMeApi;

public class CapitalsListActivity extends ListActivity
{
  CityAdapter mCityAdapter;

  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.capitals_list_activity);
    
    mCityAdapter = new CityAdapter(this, City.CAPITALS);
    setListAdapter(mCityAdapter);
    
    findViewById(R.id.btn_all).setOnClickListener(new OnClickListener()
    {
      @Override
      public void onClick(View v) { showCityOnMWMMap(City.CAPITALS); }
    });
  }
  
  
  @Override
  protected void onListItemClick(ListView l, View v, int position, long id)
  {
    showCityOnMWMMap(mCityAdapter.getItem(position));
  }
  
  private void showCityOnMWMMap(City ... cities)
  {
    MWMPoint[] points = new MWMPoint[cities.length];
    for (int i = 0; i < cities.length; i++)
      points[i] = cities[i].toMWMPoint();
    
    final String title = cities.length == 1 ? cities[0].getName() : "Capitals of the World";
    MapsWithMeApi.showPointsOnMap(this, title, CityDetailsActivity.getPendingIntent(this), points);
  }
  
  private static class CityAdapter extends ArrayAdapter<City>
  {
 
    private final City[] data;
    
    public CityAdapter(Context context, City[] cities)
    {
      super(context, android.R.layout.simple_list_item_2, android.R.id.text1, cities);
      data = cities;
    }
    
    @Override
    public View getView(int position, View convertView, ViewGroup parent)
    {
      final View view = super.getView(position, convertView, parent);
      final TextView subText = (TextView) view.findViewById(android.R.id.text2);
      final City city = data[position];
      subText.setText(city.getCountryCode() + "/" + city.getTimeZone());
      return view;
    }
  }
}
