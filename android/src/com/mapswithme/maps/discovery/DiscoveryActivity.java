package com.mapswithme.maps.discovery;

import android.app.Activity;
import android.content.Intent;
import android.support.annotation.NonNull;
import android.support.v4.app.Fragment;

import com.mapswithme.maps.base.BaseMwmFragmentActivity;

public class DiscoveryActivity extends BaseMwmFragmentActivity
{
  public static void start(@NonNull Activity activity)
  {
    final Intent i = new Intent(activity, DiscoveryActivity.class);
    activity.startActivity(i);
  }

  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return DiscoveryFragment.class;
  }
}
