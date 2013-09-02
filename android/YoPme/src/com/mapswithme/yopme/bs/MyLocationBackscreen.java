package com.mapswithme.yopme.bs;

import android.location.Location;

import com.yotadevices.sdk.BSActivity;
import com.yotadevices.sdk.BSMotionEvent;

public class MyLocationBackscreen extends BackscreenBase
{

  public MyLocationBackscreen(BSActivity bsActivity)
  {
    super(bsActivity);
    updateData(mMapDataProvider.getMyPositionData(0, 0, 0));
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
    updateData(mMapDataProvider.getMyPositionData(location.getLatitude(), location.getLongitude(), 9));
  }

}
