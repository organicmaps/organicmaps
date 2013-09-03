package com.mapswithme.yopme.bs;

import android.location.Location;

import com.mapswithme.yopme.State;
import com.mapswithme.yopme.map.MapData;
import com.mapswithme.yopme.map.MapDataProvider;
import com.yotadevices.sdk.BSActivity;
import com.yotadevices.sdk.BSMotionEvent;

public class MyLocationBackscreen extends BackscreenBase
{

  public MyLocationBackscreen(BSActivity bsActivity, State state)
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
    mock();
  }

  private void mock()
  {
    final Location location = new Location("");
    final MapData data = mMapDataProvider
        .getMyPositionData(location.getLatitude(), location.getLongitude(), MapDataProvider.ZOOM_DEFAULT);

    mState = new State(mState.getMode(), data.getPoint(), data.getBitmap());

    State.write(mBsActivity, mState);
    updateView(mState);
  }
}
