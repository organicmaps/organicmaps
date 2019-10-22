package com.mapswithme.maps.widget;

import androidx.annotation.NonNull;
import androidx.viewpager.widget.ViewPager;
import androidx.appcompat.app.AppCompatActivity;

public class PageViewProviderFactory
{
  @NonNull
  static PageViewProvider defaultProvider(@NonNull AppCompatActivity activity,
                                          @NonNull ViewPager pager)
  {
    return new FragmentPageViewProvider(activity.getSupportFragmentManager(), pager);
  }
}
