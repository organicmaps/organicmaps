package com.mapswithme.maps.discovery;

import android.app.Activity;
import android.content.Intent;
import android.support.annotation.NonNull;
import android.support.v4.app.Fragment;

import com.mapswithme.maps.activity.CustomNavigateUpListener;
import com.mapswithme.maps.base.BaseMwmFragmentActivity;
import com.mapswithme.maps.bookmarks.data.MapObject;

public class DiscoveryActivity extends BaseMwmFragmentActivity
    implements CustomNavigateUpListener, DiscoveryFragment.DiscoveryListener
{
  public static final String EXTRA_DISCOVERY_OBJECT = "extra_discovery_object";
  public static final String ACTION_ROUTE_TO = "action_route_to";
  public static final String ACTION_SHOW_ON_MAP = "action_show_on_map";

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

  private void setResult(@NonNull MapObject object, @NonNull Intent intent)
  {
    intent.putExtra(DiscoveryActivity.EXTRA_DISCOVERY_OBJECT, object);
    setResult(Activity.RESULT_OK, intent);
    finish();
  }
}
