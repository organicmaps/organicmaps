package com.mapswithme.maps.base;

import com.mapswithme.maps.MWMApplication;
import com.mapswithme.maps.state.SuppotedState;
import com.mapswithme.util.Utils;
import com.mapswithme.util.statistics.Statistics;

import android.app.ActionBar;
import android.app.ListActivity;
import android.content.Context;
import android.os.Bundle;
import android.support.v4.app.ListFragment;
import android.view.MenuItem;
import android.view.inputmethod.InputMethodManager;

/**
 * TODO use simple activity and {@link ListFragment} instead.
 */
public class MapsWithMeBaseListActivity extends ListActivity
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
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);

    if (Utils.apiEqualOrGreaterThan(11))
    {
      // http://stackoverflow.com/questions/6867076/getactionbar-returns-null
      final ActionBar bar = getActionBar();
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
      final InputMethodManager im = (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
      im.hideSoftInputFromWindow(getCurrentFocus().getWindowToken(), InputMethodManager.HIDE_NOT_ALWAYS);
      onBackPressed();
      return true;
    }
    else
      return super.onOptionsItemSelected(item);
  }

}
