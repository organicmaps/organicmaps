package com.mapswithme.maps.bookmarks;

import android.support.v4.app.Fragment;

import com.mapswithme.maps.base.BaseMwmFragmentActivity;

public class BookmarkSubscriptionActivity extends BaseMwmFragmentActivity
{
  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return BookmarkSubscriptionFragment.class;
  }

  @Override
  protected boolean useTransparentStatusBar()
  {
    return false;
  }
}
