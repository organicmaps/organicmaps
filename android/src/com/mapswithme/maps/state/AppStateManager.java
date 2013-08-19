package com.mapswithme.maps.state;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.api.ParsedMmwRequest;


public class AppStateManager
{

  private SuppotedState mCurrentState = SuppotedState.DEFAULT_MAP;

  public SuppotedState getCurrentState() { return mCurrentState; }

  public synchronized void transitionTo(SuppotedState state)
  {
    mCurrentState = state;

    // transition rules
    if (state == SuppotedState.DEFAULT_MAP)
    {
      Framework.clearApiPoints();
      ParsedMmwRequest.setCurrentRequest(null);
    }
  }

}
