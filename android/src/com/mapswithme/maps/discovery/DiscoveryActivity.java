package com.mapswithme.maps.discovery;

import android.app.Activity;
import android.content.Intent;
import android.support.annotation.NonNull;
import android.support.v4.app.Fragment;

import com.mapswithme.maps.R;
import com.mapswithme.maps.activity.CustomNavigateUpListener;
import com.mapswithme.maps.base.BaseMwmFragmentActivity;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.search.FilterActivity;

public class DiscoveryActivity extends BaseMwmFragmentActivity
    implements CustomNavigateUpListener, DiscoveryFragment.DiscoveryListener
{
  public static final String EXTRA_DISCOVERY_OBJECT = "extra_discovery_object";
  public static final String EXTRA_FILTER_SEARCH_QUERY = "extra_filter_search_query";
  public static final String ACTION_ROUTE_TO = "action_route_to";
  public static final String ACTION_SHOW_ON_MAP = "action_show_on_map";
  public static final String ACTION_SHOW_FILTER_RESULTS = "action_show_filter_results";

  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return DiscoveryFragment.class;
  }

  @Override
  public void customOnNavigateUp()
  {
    finish();
  }

  @Override
  public void onRouteToDiscoveredObject(@NonNull MapObject object)
  {
    Intent intent = new Intent(ACTION_ROUTE_TO);
    setResult(object, intent);
  }

  @Override
  public void onShowDiscoveredObject(@NonNull MapObject object)
  {
    Intent intent = new Intent(ACTION_SHOW_ON_MAP);
    setResult(object, intent);
  }

  @Override
  public void onShowFilter()
  {
    FilterActivity.startForResult(this, null, null, FilterActivity.REQ_CODE_FILTER);
  }

  private void setResult(@NonNull MapObject object, @NonNull Intent intent)
  {
    intent.putExtra(DiscoveryActivity.EXTRA_DISCOVERY_OBJECT, object);
    setResult(Activity.RESULT_OK, intent);
    finish();
  }

  @Override
  protected void onActivityResult(int requestCode, int resultCode, Intent data)
  {
    super.onActivityResult(requestCode, resultCode, data);

    if (resultCode != Activity.RESULT_OK)
      return;

    switch (requestCode)
    {
      case FilterActivity.REQ_CODE_FILTER:
        if (data == null)
          return;

        data.setAction(ACTION_SHOW_FILTER_RESULTS);
        data.putExtra(EXTRA_FILTER_SEARCH_QUERY, getString(R.string.hotel));
        setResult(Activity.RESULT_OK, data);
        finish();
        break;
    }
  }
}
