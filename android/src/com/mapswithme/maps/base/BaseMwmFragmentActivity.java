package com.mapswithme.maps.base;

import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentTransaction;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.view.MenuItem;

import com.mapswithme.util.ViewServer;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.util.Utils;
import com.mapswithme.util.statistics.Statistics;

import ru.mail.android.mytracker.MRMyTracker;

public class BaseMwmFragmentActivity extends AppCompatActivity
{
  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    final int layoutId = getContentLayoutResId();
    if (layoutId != 0)
      setContentView(layoutId);

    // Use full-screen on Kindle Fire only
    if (Utils.isAmazonDevice())
    {
      getWindow().addFlags(android.view.WindowManager.LayoutParams.FLAG_FULLSCREEN);
      getWindow().clearFlags(android.view.WindowManager.LayoutParams.FLAG_FORCE_NOT_FULLSCREEN);
    }

    MwmApplication.get().initStats();
    ViewServer.get(this).addWindow(this);

    attachDefaultFragment();
  }

  @Override
  protected void onDestroy()
  {
    super.onDestroy();
    ViewServer.get(this).removeWindow(this);
  }

  @Override
  protected void onStart()
  {
    super.onStart();
    Statistics.INSTANCE.startActivity(this);
    MRMyTracker.onStartActivity(this);
    org.alohalytics.Statistics.onStart(this);
  }

  @Override
  protected void onStop()
  {
    org.alohalytics.Statistics.onStop(this);
    Statistics.INSTANCE.stopActivity(this);
    super.onStop();

    MRMyTracker.onStopActivity(this);
  }

  @Override
  public boolean onOptionsItemSelected(MenuItem item)
  {
    if (item.getItemId() == android.R.id.home)
    {
      onBackPressed();
      return true;
    }
    else
      return super.onOptionsItemSelected(item);
  }

  @Override
  protected void onResume()
  {
    super.onResume();
    org.alohalytics.Statistics.logEvent("$onResume", this.getClass().getSimpleName()
        + ":" + com.mapswithme.util.UiUtils.deviceOrientationAsString(this));
    ViewServer.get(this).setFocusedWindow(this);
  }

  @Override
  protected void onPause()
  {
    super.onPause();
    org.alohalytics.Statistics.logEvent("$onPause", this.getClass().getSimpleName());
  }

  protected Toolbar getToolbar()
  {
    return (Toolbar) findViewById(R.id.toolbar);
  }

  protected void displayToolbarAsActionBar()
  {
    setSupportActionBar(getToolbar());
  }

  /**
   * Override to set custom content view.
   * @return layout resId.
   */
  protected int getContentLayoutResId()
  {
    return 0;
  }

  protected void attachDefaultFragment()
  {
    Class<? extends Fragment> clazz = getFragmentClass();
    if (clazz != null)
      replaceFragment(clazz, false, getIntent().getExtras());
  }

  /**
   * Replace attached fragment with the new one.
   */
  public void replaceFragment(Class<? extends Fragment> fragmentClass, boolean addToBackStack, Bundle args)
  {
    final int resId = getFragmentContentResId();
    if (resId <= 0 || findViewById(resId) == null)
      throw new IllegalStateException("Fragment can't be added, since getFragmentContentResId() isn't implemented or returns wrong resourceId.");

    final FragmentTransaction transaction = getSupportFragmentManager().beginTransaction();

    String name = fragmentClass.getName();
    final Fragment fragment = Fragment.instantiate(this, name, args);
    transaction.replace(resId, fragment, name);
    if (addToBackStack)
      transaction.addToBackStack(null);
    transaction.commit();
  }

  /**
   * Override to automatically attach fragment in onCreate. Tag applied to fragment in back stack is set to fragment name, too.
   * WARNING : if custom layout for activity is set, getFragmentContentResId() must be implemented, too.
   * @return class of the fragment, eg FragmentClass.getClass()
   */
  protected Class<? extends Fragment> getFragmentClass()
  {
    return null;
  }

  /**
   * Get resource id for the fragment. That must be implemented to return correct resource id, if custom layout is set.
   * @return resourceId for the fragment
   */
  protected int getFragmentContentResId()
  {
    return android.R.id.content;
  }
}
