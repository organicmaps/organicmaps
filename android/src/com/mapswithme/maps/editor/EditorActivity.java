package com.mapswithme.maps.editor;

import android.app.Activity;
import android.content.Intent;
import android.support.annotation.NonNull;
import android.support.v4.app.Fragment;

import com.mapswithme.maps.base.BaseMwmFragmentActivity;
import com.mapswithme.maps.bookmarks.data.MapObject;

public class EditorActivity extends BaseMwmFragmentActivity
{
  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return EditorHostFragment.class;
  }

  public static void start(@NonNull Activity activity, @NonNull MapObject point)
  {
    final Intent intent = new Intent(activity, EditorActivity.class);
    intent.putExtra(EditorHostFragment.EXTRA_MAP_OBJECT, point);
    activity.startActivity(intent);
  }
}
