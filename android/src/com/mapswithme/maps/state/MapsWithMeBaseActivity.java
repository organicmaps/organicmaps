package com.mapswithme.maps.state;

import android.app.Activity;

import com.mapswithme.maps.MWMApplication;

public class MapsWithMeBaseActivity extends Activity
{

  protected SuppotedState mState;

  public SuppotedState getState()                    { return mState; }
  public void          setState(SuppotedState state) { mState = state; }


  @Override
  protected void onResume()
  {
    super.onResume();
    updateState();
    setViewFromState(mState);
  }

  public void setViewFromState(SuppotedState state) {};

  public void updateState()
  {
    mState = getMwmApplication().getAppState();
  }


  public MWMApplication getMwmApplication()
  {
    return (MWMApplication)getApplication();
  }

}
