package com.mapswithme.yopme.bs;

import android.location.Location;

import com.mapswithme.maps.api.MWMPoint;
import com.yotadevices.sdk.BSActivity;
import com.yotadevices.sdk.BSMotionEvent;

public class PoiLocationBackscreen extends BackscreenBase
{
  private final MWMPoint mPoint;
  public PoiLocationBackscreen(BSActivity bsActivity, MWMPoint point)
  {
    super(bsActivity);
    mPoint = point;
    updateData(mMapDataProvider.getPOIData(mPoint ,0));
  }

  @Override
  public void onMotionEvent(BSMotionEvent motionEvent)
  {
    super.onMotionEvent(motionEvent);
    invalidate();
  }

  @Override
  public void onLocationChanged(Location location)
  {
    updateData(mMapDataProvider.getPOIData(mPoint ,0));
  }

}
