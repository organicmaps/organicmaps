package com.mapswithme.maps.bookmarks;

import android.os.Bundle;
import android.support.v7.widget.Toolbar;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmFragmentActivity;
import com.mapswithme.util.UiUtils;

public class BookmarkCategoriesActivity extends BaseMwmFragmentActivity
{
  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);

    final Toolbar toolbar = getToolbar();
    toolbar.setTitle(R.string.bookmark_sets);
    UiUtils.showHomeUpButton(toolbar);
    displayToolbarAsActionBar();
  }

  @Override
  protected int getContentLayoutResId()
  {
    return R.layout.activity_fragment_and_toolbar;
  }

  @Override
  protected String getFragmentClassName()
  {
    return BookmarkCategoriesFragment.class.getName();
  }

  @Override
  protected int getFragmentContentResId()
  {
    return R.id.fragment_container;
  }
}