package com.mapswithme.maps.settings;

import android.app.Activity;
import android.content.Intent;
import androidx.annotation.NonNull;
import androidx.fragment.app.Fragment;

import com.mapswithme.maps.MwmActivity;
import com.mapswithme.maps.base.BaseMwmFragmentActivity;

public class DrivingOptionsActivity extends BaseMwmFragmentActivity
{
  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return DrivingOptionsFragment.class;
  }

  public static void start(@NonNull Activity activity)
  {
    Intent intent = new Intent(activity, DrivingOptionsActivity.class);
    activity.startActivityForResult(intent, MwmActivity.REQ_CODE_DRIVING_OPTIONS);
  }
}
