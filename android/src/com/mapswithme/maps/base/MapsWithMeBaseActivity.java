package com.mapswithme.maps.base;

import android.annotation.SuppressLint;
import android.app.ActionBar;
import android.app.Activity;
import android.os.Bundle;
import android.support.v4.app.FragmentActivity;
import android.view.MenuItem;
import android.view.inputmethod.InputMethodManager;

import com.mapswithme.maps.MWMApplication;
import com.mapswithme.util.Utils;
import com.mapswithme.util.statistics.Statistics;

public class MapsWithMeBaseActivity extends FragmentActivity
{
  @Override
  protected void onResume()
  {
    super.onResume();
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

  @SuppressLint("NewApi")
  @Override
  protected void onCreate(Bundle arg0)
  {
    super.onCreate(arg0);

    if (Utils.apiEqualOrGreaterThan(11))
    {
      // http://stackoverflow.com/questions/6867076/getactionbar-returns-null
      final ActionBar bar = getActionBar();
      if (bar != null)
        bar.setDisplayHomeAsUpEnabled(true);
    }
  }

  public MWMApplication getMwmApplication()
  {
    return (MWMApplication) getApplication();
  }

  @Override
  public boolean onOptionsItemSelected(MenuItem item)
  {
    if (item.getItemId() == android.R.id.home)
    {
      final InputMethodManager imm = (InputMethodManager) getSystemService(Activity.INPUT_METHOD_SERVICE);
      imm.toggleSoftInput(InputMethodManager.HIDE_IMPLICIT_ONLY, 0);
      onBackPressed();
      return true;
    }
    else
      return super.onOptionsItemSelected(item);
  }

}
