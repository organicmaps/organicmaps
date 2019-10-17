package com.mapswithme.maps.widget;

import android.support.annotation.NonNull;
import android.support.v4.view.ViewPager;
import android.support.v7.app.AppCompatActivity;

public class PageViewProviderFactory
{
  @NonNull
  static PageViewProvider defaultProvider(@NonNull AppCompatActivity activity,
                                          @NonNull ViewPager pager)
  {
    return new FragmentPageViewProvider(activity.getSupportFragmentManager(), pager);
  }
}
