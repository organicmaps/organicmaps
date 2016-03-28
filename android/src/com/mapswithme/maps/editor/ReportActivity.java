package com.mapswithme.maps.editor;

import android.app.Activity;
import android.content.Intent;
import android.support.annotation.NonNull;
import android.support.v4.app.Fragment;

import com.mapswithme.maps.base.BaseMwmFragmentActivity;
import com.mapswithme.maps.bookmarks.data.MapObject;

public class ReportActivity extends BaseMwmFragmentActivity
{
  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return ReportFragment.class;
  }

  public static void start(@NonNull Activity activity, MapObject point)
  {
    final Intent intent = new Intent(activity, ReportActivity.class)
                              .putExtra(ReportFragment.EXTRA_LAT, point.getLat())
                              .putExtra(ReportFragment.EXTRA_LON, point.getLon());
    activity.startActivity(intent);
  }
}
