package com.mapswithme.maps.base;

import android.app.ActionBar;
import android.app.Activity;
import android.os.Build;
import android.os.Bundle;
import android.support.v4.app.FragmentActivity;
import android.view.MenuItem;
import android.view.inputmethod.InputMethodManager;

import com.mapswithme.maps.MWMApplication;
import com.mapswithme.maps.state.SuppotedState;
import com.mapswithme.util.Statistics;
import com.mapswithme.util.Utils;

public class MapsWithMeBaseActivity extends FragmentActivity
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

  @Override
  protected void onStart()
  {
    super.onStart();
    Statistics.INSTANCE.startActivity(this);
  }

  @Override
  protected void onStop()
  {
    Statistics.INSTANCE.stopActivity(this);
    super.onStop();
  }

  @Override
  protected void onCreate(Bundle arg0)
  {
    super.onCreate(arg0);

    if (Utils.apiEqualOrGreaterThan(11))
    {
      // http://stackoverflow.com/questions/6867076/getactionbar-returns-null
      ActionBar bar = getActionBar();
      if (bar != null)
        bar.setDisplayHomeAsUpEnabled(true);
    }
  }

  /**
   *  Method to be overridden from child.
   *  Set specific view for activity here.
   *
   * @param state
   */
  public void setViewFromState(SuppotedState state) {};

  public void updateState()
  {
    mState = getMwmApplication().getAppState();
  }


  public MWMApplication getMwmApplication()
  {
    return (MWMApplication)getApplication();
  }

  @Override
  public boolean onOptionsItemSelected(MenuItem item)
  {
    if (item.getItemId() == android.R.id.home)
    {
      InputMethodManager imm = (InputMethodManager) getSystemService(Activity.INPUT_METHOD_SERVICE);
      imm.toggleSoftInput(InputMethodManager.HIDE_IMPLICIT_ONLY, 0);
      onBackPressed();
      return true;
    }
    else
      return super.onOptionsItemSelected(item);
  }

}
