package com.mapswithme.maps.editor;

import android.app.Activity;
import android.content.Intent;
import android.support.v4.app.Fragment;

import com.mapswithme.maps.base.BaseMwmFragmentActivity;
import com.mapswithme.maps.bookmarks.data.MapObject;

public class EditorActivity extends BaseMwmFragmentActivity
{
  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return EditorFragment.class;
  }

  @Override
  public void onBackPressed()
  {
    final EditorFragment fragment = (EditorFragment)getSupportFragmentManager().findFragmentByTag(getFragmentClass().getName());
    if ((fragment == null) || !fragment.isAdded() || !fragment.onBackPressed())
      super.onBackPressed();
  }

  public static void start(Activity activity, MapObject point)
  {
    activity.startActivity(new Intent(activity, EditorActivity.class)
                               .putExtra(EditorFragment.EXTRA_POINT, point));
  }
}
