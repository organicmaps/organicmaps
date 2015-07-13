package com.mapswithme.maps.search;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentTransaction;
import android.support.v4.app.NavUtils;

import com.mapswithme.maps.activity.CustomNavigateUpListener;
import com.mapswithme.maps.base.BaseMwmFragmentActivity;

public class SearchActivity extends BaseMwmFragmentActivity implements CustomNavigateUpListener
{
  public static final String EXTRA_QUERY = "search_query";

  public static void startWithQuery(Context context, String query)
  {
    final Intent i = new Intent(context, SearchActivity.class);
    i.putExtra(EXTRA_QUERY, query);
    context.startActivity(i);
  }

  @Override
  protected String getFragmentClassName()
  {
    return SearchFragment.class.getName();
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
}
