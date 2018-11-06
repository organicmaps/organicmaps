package com.mapswithme.maps.ugc.routes;

import android.app.Activity;
import android.support.annotation.NonNull;
import android.support.v4.app.Fragment;

import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;

public class UgcRouteEditSettingsActivity extends BaseUgcRouteActivity
{
  public static final int REQUEST_CODE = 107;

  @Override
  protected int getContentLayoutResId()
  {
    return R.layout.fragment_container_layout;
  }

  @Override
  protected int getFragmentContentResId()
  {
    return R.id.fragment_container;
  }

  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return UgcRouteEditSettingsFragment.class;
  }

  public static void startForResult(@NonNull Activity activity, @NonNull BookmarkCategory category)
  {
    startForResult(activity, category, UgcRouteEditSettingsActivity.class, REQUEST_CODE);
  }
}
