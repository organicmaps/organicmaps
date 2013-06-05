
package com.mapswithme.capitals;

import android.app.Activity;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.TextView;

import com.mapswithme.maps.api.MWMResponse;
import com.mapswithme.maps.api.MapsWithMeApi;

public class CityDetailsActivity extends Activity
{
  public static String EXTRA_FROM_MWM = "from-maps-with-me";

  private TextView mName;
  private TextView mAltNames;
  private TextView mCountry;

  private TextView mLat;
  private TextView mLon;
  private TextView mElev;

  private TextView mPopulation;
  private TextView mTimeZone;

  private City mCity;

  public static PendingIntent getPendingIntent(Context context)
  {
    final Intent i = new Intent(context, CityDetailsActivity.class);
    i.putExtra(EXTRA_FROM_MWM, true);
    return PendingIntent.getActivity(context, 0, i, 0);
  }

  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.city_details_activity);

    mName = (TextView) findViewById(R.id.name);
    mAltNames = (TextView) findViewById(R.id.altNames);
    mCountry = (TextView) findViewById(R.id.cCode);

    mLat = (TextView) findViewById(R.id.lat);
    mLon = (TextView) findViewById(R.id.lon);
    mElev = (TextView) findViewById(R.id.elevation);

    mPopulation = (TextView) findViewById(R.id.population);
    mTimeZone = (TextView) findViewById(R.id.timeZone);

    findViewById(R.id.showOnMap).setOnClickListener(new OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        MapsWithMeApi.showPointsOnMap(CityDetailsActivity.this,
                                      mCity.getName(),
                                      CityDetailsActivity.getPendingIntent(CityDetailsActivity.this),
                                      mCity.toMWMPoint());
      }
    });

    handleIntent(getIntent());
  }

  @Override
  protected void onNewIntent(Intent intent)
  {
    super.onNewIntent(intent);
    handleIntent(intent);
  }

  private void handleIntent(Intent intent)
  {
    if (intent.getBooleanExtra(EXTRA_FROM_MWM, false))
    {
      final MWMResponse response = MWMResponse.extractFromIntent(this, intent);
      mCity = City.fromMWMPoint(response.getPoint());

      if (mCity != null)
      {
        mName.setText(mCity.getName());
        mAltNames.setText(mCity.getAltNames());
        mCountry.setText(mCity.getCountryCode());

        mLat.setText(mCity.getLat() + "");
        mLon.setText(mCity.getLon() + "");
        final String evel = mCity.getElevation() != -9999 ? String.valueOf(mCity.getElevation()) : "No Data";
        mElev.setText(evel);

        final String popul = mCity.getPopulation() != -1 ? String.valueOf(mCity.getPopulation()) : "No Data";
        mPopulation.setText(popul);
        mTimeZone.setText(mCity.getTimeZone());
      }
    }
  }
}
