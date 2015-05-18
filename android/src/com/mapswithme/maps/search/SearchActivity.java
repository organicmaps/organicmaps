package com.mapswithme.maps.search;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentTransaction;

import com.mapswithme.maps.base.BaseMwmFragmentActivity;

public class SearchActivity extends BaseMwmFragmentActivity
{
  public static final String EXTRA_QUERY = "search_query";

  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);

    FragmentTransaction transaction = getSupportFragmentManager().beginTransaction();
    Fragment fragment = Fragment.instantiate(this, SearchFragment.class.getName(), getIntent().getExtras());
    transaction.replace(android.R.id.content, fragment, "fragment");
    transaction.commit();
  }

  public static void startForSearch(Context context, String query, int scope)
  {
    final Intent i = new Intent(context, SearchActivity.class);
    i.putExtra(EXTRA_QUERY, query);
    context.startActivity(i);
  }

  public static void startForSearch(Context context, String query)
  {
    final Intent i = new Intent(context, SearchActivity.class);
    i.putExtra(EXTRA_QUERY, query);
    context.startActivity(i);
  }
}
