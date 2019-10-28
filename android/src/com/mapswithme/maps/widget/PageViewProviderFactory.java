package com.mapswithme.maps.widget;

import androidx.annotation.NonNull;
import androidx.viewpager.widget.ViewPager;
import androidx.appcompat.app.FragmentActivity;

public class PageViewProviderFactory
{
  @NonNull
  static PageViewProvider defaultProvider(@NonNull FragmentActivity activity,
                                          @NonNull ViewPager pager)
  {
    return new FragmentPageViewProvider(activity.getSupportFragmentManager(), pager);
  }
}
