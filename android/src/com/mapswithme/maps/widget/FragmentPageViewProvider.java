package com.mapswithme.maps.widget;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentManager;
import androidx.fragment.app.FragmentPagerAdapter;
import androidx.viewpager.widget.ViewPager;
import android.view.View;

import java.util.NoSuchElementException;

public class FragmentPageViewProvider implements PageViewProvider
{
  private static final String ANDROID_SWITCHER_TAG_SEGMENT = "android:switcher:";
  private static final String SEPARATOR_TAG_SEGMENT = ":";

  @NonNull
  private final FragmentManager mFragManager;
  private final int mPagerId;

  FragmentPageViewProvider(@NonNull FragmentManager fragManager, @NonNull ViewPager pager)
  {
    checkAdapterClass(pager);
    mFragManager = fragManager;
    mPagerId = pager.getId();
  }

  private static void checkAdapterClass(@NonNull ViewPager pager)
  {
    try
    {
      FragmentPagerAdapter adapter = (FragmentPagerAdapter) pager.getAdapter();
      if (adapter == null)
        throw new IllegalStateException("Adapter not found");
    }
    catch (ClassCastException e)
    {
      throw new IllegalStateException("Adapter has to be FragmentPagerAdapter or its descendant");
    }
  }

  @Nullable
  @Override
  public View findViewByIndex(int index)
  {
    String tag = makePagerFragmentTag(index);
    Fragment page = getSupportFragmentManager().findFragmentByTag(tag);
    if (page == null)
      throw new NoSuchElementException("No such element for tag  : " + tag);

    return page.getView();
  }

  private int getId()
  {
    return mPagerId;
  }

  @NonNull
  private FragmentManager getSupportFragmentManager()
  {
    return mFragManager;
  }

  @NonNull
  private String makePagerFragmentTag(int index)
  {
    return ANDROID_SWITCHER_TAG_SEGMENT + getId() + SEPARATOR_TAG_SEGMENT + index;
  }
}
