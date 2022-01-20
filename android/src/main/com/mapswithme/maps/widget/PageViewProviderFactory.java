package com.mapswithme.maps.widget;

import androidx.annotation.NonNull;
import androidx.fragment.app.FragmentActivity;
import androidx.viewpager.widget.ViewPager;

class PageViewProviderFactory
{
  @NonNull
  static PageViewProvider defaultProvider(@NonNull FragmentActivity activity,
                                          @NonNull ViewPager pager)
  {
    return new FragmentPageViewProvider(activity.getSupportFragmentManager(), pager);
  }
}
