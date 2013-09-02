package com.mapswithme.yopme.bs;

import com.yotadevices.sdk.BSActivity;
import com.yotadevices.sdk.BSMotionEvent;

public class PoiLocationBackscreen extends BackscreenBase
{

  public PoiLocationBackscreen(BSActivity bsActivity)
  {
    super(bsActivity);
    updateData(mMapDataProvider.getPOIData(null ,0));
  }

  @Override
  public void onMotionEvent(BSMotionEvent motionEvent)
  {
    invalidate();
  }

}
