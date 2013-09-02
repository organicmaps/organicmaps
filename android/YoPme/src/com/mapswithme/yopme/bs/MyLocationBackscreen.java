package com.mapswithme.yopme.bs;

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
    invalidate();
  }

}
