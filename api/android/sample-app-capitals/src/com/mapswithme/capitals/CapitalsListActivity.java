/******************************************************************************
   Copyright (c) 2013, MapsWithMe GmbH All rights reserved.

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

  Redistributions of source code must retain the above copyright notice, this list
  of conditions and the following disclaimer. Redistributions in binary form must
  reproduce the above copyright notice, this list of conditions and the following
  disclaimer in the documentation and/or other materials provided with the
  distribution. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
  PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
  OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
  IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
  OF SUCH DAMAGE.
******************************************************************************/
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
