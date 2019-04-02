package com.mapswithme.maps.settings;

import android.content.Intent;
import android.support.annotation.NonNull;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentActivity;

import com.mapswithme.maps.base.BaseMwmFragmentActivity;

public class DrivingOptionsActivity extends BaseMwmFragmentActivity
{
  public static final String BUNDLE_REQUIRE_OPTIONS_MENU = "require_options_menu";

  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return DrivingOptionsFragment.class;
  }

  public static void start(@NonNull FragmentActivity activity)
  {
    Intent intent = new Intent(activity, DrivingOptionsActivity.class)
        .putExtra(BUNDLE_REQUIRE_OPTIONS_MENU, true);
    activity.startActivity(intent);
  }
}
