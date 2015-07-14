package com.mapswithme.maps.search;

import android.content.Context;
import android.content.Intent;

import com.mapswithme.maps.base.BaseMwmFragmentActivity;

public class SearchActivity extends BaseMwmFragmentActivity
{
  public static final String EXTRA_QUERY = "search_query";

  @Override
  protected String getFragmentClassName()
  {
    return SearchFragment.class.getName();
  }

  public static void startForSearch(Context context, String query)
  {
    final Intent i = new Intent(context, SearchActivity.class);
    i.putExtra(EXTRA_QUERY, query);
    context.startActivity(i);
  }
}
