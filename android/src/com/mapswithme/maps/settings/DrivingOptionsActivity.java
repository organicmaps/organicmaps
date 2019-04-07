package com.mapswithme.maps.settings;

import android.app.Activity;
import android.content.Intent;
import android.support.annotation.NonNull;
import android.support.v4.app.Fragment;

import com.mapswithme.maps.MwmActivity;
import com.mapswithme.maps.base.BaseMwmFragmentActivity;

public class DrivingOptionsActivity extends BaseMwmFragmentActivity
{
  public static final String BUNDLE_REQUIRE_OPTIONS_MENU = "require_options_menu";

  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return DrivingOptionsFragment.class;
  }

  public static void start(@NonNull Activity activity)
  {
    Intent intent = new Intent(activity, DrivingOptionsActivity.class)
        .putExtra(BUNDLE_REQUIRE_OPTIONS_MENU, true);
    activity.startActivityForResult(intent, MwmActivity.REQ_CODE_DRIVING_OPTIONS);
  }
}
