package com.mapswithme.maps.bookmarks;

import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v7.widget.Toolbar;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmFragmentActivity;
import com.mapswithme.util.UiUtils;

public class BookmarkListActivity extends BaseMwmFragmentActivity
{
  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);

    final Toolbar toolbar = getToolbar();
    toolbar.setTitle(R.string.bookmarks);
    UiUtils.showHomeUpButton(toolbar);
    displayToolbarAsActionBar();
  }

  @Override
  protected int getContentLayoutResId()
  {
    return R.layout.activity_fragment_and_toolbar;
  }

  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return BookmarksListFragment.class;
  }

  @Override
  protected int getFragmentContentResId()
  {
    return R.id.fragment_container;
  }
}
