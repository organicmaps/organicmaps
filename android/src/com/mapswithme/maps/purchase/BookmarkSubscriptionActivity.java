package com.mapswithme.maps.purchase;

import android.support.v4.app.Fragment;

import com.mapswithme.maps.base.BaseMwmFragmentActivity;
import com.mapswithme.maps.purchase.BookmarkSubscriptionFragment;

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
