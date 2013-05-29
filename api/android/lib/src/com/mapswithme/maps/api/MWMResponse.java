package com.mapswithme.maps.api;

import android.content.Context;
import android.content.Intent;

// TODO add javadoc for public interface
public class MWMResponse
{
  private MWMPoint mPoint;

  public MWMPoint getPoint()     { return mPoint; }
  public boolean  hasPoint()     { return mPoint != null; }

  @Override
  public String toString()
  {
    return "MWMResponse [mSelectedPoint=" + mPoint + "]";
  }

  static MWMResponse extractFromIntent(Context context, Intent intent)
  {
    final MWMResponse response = new MWMResponse();
    // parse status
    // parse point
    final double lat = intent.getDoubleExtra(Const.EXTRA_MWM_RESPONSE_POINT_LAT, 0);
    final double lon = intent.getDoubleExtra(Const.EXTRA_MWM_RESPONSE_POINT_LON, 0);
    final String name = intent.getStringExtra(Const.EXTRA_MWM_RESPONSE_POINT_NAME);
    final String id = intent.getStringExtra(Const.EXTRA_MWM_RESPONSE_POINT_ID);
    response.mPoint = new MWMPoint(lat, lon, name, id);
    
    return response;
  }

  private MWMResponse() {}
}
