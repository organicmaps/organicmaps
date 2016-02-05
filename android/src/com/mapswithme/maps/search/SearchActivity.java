package com.mapswithme.maps.search;

import android.content.Context;
import android.content.Intent;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.NavUtils;

import com.mapswithme.maps.activity.CustomNavigateUpListener;
import com.mapswithme.maps.base.BaseMwmFragmentActivity;
import com.mapswithme.maps.base.OnBackPressListener;

public class SearchActivity extends BaseMwmFragmentActivity implements CustomNavigateUpListener
{
  public static final String EXTRA_QUERY = "search_query";

  public static void start(Context context, String query)
  {
    final Intent i = new Intent(context, SearchActivity.class);
    i.putExtra(EXTRA_QUERY, query);
    context.startActivity(i);
  }

  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return SearchFragment.class;
  }

  @Override
  public void customOnNavigateUp()
  {
    final FragmentManager manager = getSupportFragmentManager();
    if (manager.getBackStackEntryCount() == 0)
    {
      NavUtils.navigateUpFromSameTask(this);
      return;
    }

    manager.popBackStack();
  }

  @Override
  public void onBackPressed()
  {
    for (Fragment f : getSupportFragmentManager().getFragments())
      if ((f instanceof OnBackPressListener) && ((OnBackPressListener)f).onBackPressed())
        return;

    super.onBackPressed();
  }
}
