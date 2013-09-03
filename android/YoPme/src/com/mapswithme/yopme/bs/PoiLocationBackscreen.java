package com.mapswithme.yopme.bs;

import android.location.Location;

import com.mapswithme.maps.api.MWMPoint;
import com.mapswithme.yopme.State;
import com.mapswithme.yopme.map.MapData;
import com.mapswithme.yopme.map.MapDataProvider;
import com.yotadevices.sdk.BSActivity;
import com.yotadevices.sdk.BSMotionEvent;

public class PoiLocationBackscreen extends BackscreenBase
{
  public PoiLocationBackscreen(BSActivity bsActivity, State state)
  {
    super(bsActivity, state);

    mock();
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
    super.onLocationChanged(location);
    // TODO: get location name from MWM
    mock();
  }

  private void mock()
  {
    final Location location = new Location("");
    final MapData data = mMapDataProvider.getPOIData(
        new MWMPoint(location.getLatitude(), location.getLongitude(), ""), MapDataProvider.ZOOM_DEFAULT);
    mState = new State(mState.getMode(), data.getPoint(), data.getBitmap());
    updateView(mState);
    State.write(mBsActivity, mState);
  }

}
