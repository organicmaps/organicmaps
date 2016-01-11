package com.mapswithme.maps.editor;

import android.app.Activity;
import android.content.Intent;
import android.support.v4.app.Fragment;
import android.text.TextUtils;

import com.mapswithme.maps.base.BaseMwmFragmentActivity;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.bookmarks.data.Metadata;
import com.mapswithme.maps.widget.placepage.TimetableFragment;

public class EditorActivity extends BaseMwmFragmentActivity
{
  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    // TODO use EditorFragment after is will be finished
    return TimetableFragment.class;
  }

  @Override
  public void onBackPressed()
  {
    final TimetableFragment fragment = (TimetableFragment) getSupportFragmentManager().findFragmentByTag(getFragmentClass().getName());
    if ((fragment == null) || !fragment.isAdded() || !fragment.onBackPressed())
      super.onBackPressed();
  }

  public static void start(Activity activity, MapObject point)
  {
    final Intent intent = new Intent(activity, EditorActivity.class);
    final String openHours = point.getMetadata(Metadata.MetadataType.FMD_OPEN_HOURS);
    if (!TextUtils.isEmpty(openHours))
      intent.putExtra(TimetableFragment.EXTRA_TIME, openHours);

    activity.startActivity(intent);
  }
}
