package com.mapswithme.maps.api;

import android.content.Context;
import android.content.Intent;

// TODO add javadoc for public interface
public class MWMResponse
{
  private int mStatus;
  private MWMPoint mPoint;

  public boolean isCanceled()    { return Const.STATUS_CANCEL == mStatus; }
  public boolean isSuccessful()  { return Const.STATUS_OK     == mStatus; }

  public MWMPoint getPoint()     { return mPoint; }
  public boolean  hasPoint()     { return mPoint != null; }

  @Override
  public String toString()
  {
    return "MWMResponse [mStatus=" + mStatus + ", mSelectedPoint=" + mPoint + "]";
  }

  static MWMResponse extractFromIntent(Context context, Intent intent)
  {
    final MWMResponse response = new MWMResponse();
    // parse status
    response.mStatus = intent.getIntExtra(Const.EXTRA_MWM_RESPONSE_STATUS, Const.STATUS_OK);
    // parse point
    if (intent.getBooleanExtra(Const.EXTRA_MWM_RESPONSE_HAS_POINT, false))
    {
      final double lat = intent.getDoubleExtra(Const.EXTRA_MWM_RESPONSE_POINT_LAT, 0);
      final double lon = intent.getDoubleExtra(Const.EXTRA_MWM_RESPONSE_POINT_LON, 0);
      final String name = intent.getStringExtra(Const.EXTRA_MWM_RESPONSE_POINT_NAME);
      final String id = intent.getStringExtra(Const.EXTRA_MWM_RESPONSE_POINT_ID);
      response.mPoint = new MWMPoint(lat, lon, name, id);
    }
    return response;
  }

  private MWMResponse() {}
}
