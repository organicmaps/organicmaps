package com.mapswithme.maps.base;

import android.app.Activity;
import android.os.Bundle;
import android.support.v7.app.ActionBar;
import android.support.v7.app.ActionBarActivity;
import android.view.MenuItem;
import android.view.inputmethod.InputMethodManager;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.R;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.statistics.Statistics;
import ru.mail.mrgservice.MRGService;

public class MWMFragmentActivity extends ActionBarActivity implements Framework.BuyProListener
{
  @Override
  protected void onResume()
  {
    super.onResume();
    Framework.nativeSetBuyProListener(this);
  }

  @Override
  protected void onStart()
  {
    super.onStart();
    Statistics.INSTANCE.startActivity(this);

    MRGService.instance().sendGAScreen(getClass().getName());
    MRGService.instance().onStart(this);
  }

  @Override
  protected void onStop()
  {
    Statistics.INSTANCE.stopActivity(this);
    super.onStop();

    MRGService.instance().onStop(this);
  }

  @Override
  protected void onCreate(Bundle arg0)
  {
    super.onCreate(arg0);

    final ActionBar bar = getSupportActionBar();
    if (bar != null)
      bar.setDisplayHomeAsUpEnabled(true);
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

  public void onBuyPro()
  {
    UiUtils.showBuyProDialog(MWMFragmentActivity.this, getString(R.string.routing_failed_buy_pro));
  }
}
