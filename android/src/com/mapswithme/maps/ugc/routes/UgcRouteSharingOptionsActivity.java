package com.mapswithme.maps.ugc.routes;

import android.app.Activity;
import android.support.annotation.NonNull;
import android.support.v4.app.Fragment;

import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;

public class UgcRouteSharingOptionsActivity extends BaseUgcRouteActivity
{
  public static final int REQ_CODE_SHARING_OPTIONS = 307;

  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return UgcSharingOptionsFragment.class;
  }

  @Override
  protected int getContentLayoutResId()
  {
    return R.layout.fragment_container_layout;
  }

  public static void startForResult(@NonNull Activity activity, @NonNull BookmarkCategory category)
  {
    startForResult(activity, category, UgcRouteSharingOptionsActivity.class, REQ_CODE_SHARING_OPTIONS);
  }
}
