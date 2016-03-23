package com.mapswithme.maps.editor;

import android.app.Activity;
import android.content.Intent;
import android.support.annotation.NonNull;
import android.support.v4.app.Fragment;

import com.mapswithme.maps.base.BaseMwmFragmentActivity;

public class ReportActivity extends BaseMwmFragmentActivity
{
  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return ReportFragment.class;
  }

  public static void start(@NonNull Activity activity)
  {
    final Intent intent = new Intent(activity, ReportActivity.class);
    activity.startActivity(intent);
  }
}
