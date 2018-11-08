package com.mapswithme.maps.search;

import android.content.Context;
import android.content.res.ColorStateList;
import android.support.annotation.NonNull;
import android.support.design.widget.TabLayout;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentPagerAdapter;
import android.support.v4.view.ViewPager;
import android.util.SparseArray;

import com.mapswithme.maps.R;
import com.mapswithme.util.Graphics;
import com.mapswithme.util.ThemeUtils;

import java.util.ArrayList;
import java.util.List;

class TabAdapter extends FragmentPagerAdapter
{
  enum Tab
  {
    HISTORY
    {
      @Override
      public int getTitleRes()
      {
        return R.string.history;
      }

      @Override
      public Class<? extends Fragment> getFragmentClass()
      {
        return SearchHistoryFragment.class;
      }
    },

    CATEGORIES
    {
      @Override
      public int getTitleRes()
      {
        return R.string.categories;
      }

      @Override
      public Class<? extends Fragment> getFragmentClass()
      {
        return SearchCategoriesFragment.class;
      }
    };

    public abstract int getTitleRes();
    public abstract Class<? extends Fragment> getFragmentClass();
  }

  interface OnTabSelectedListener
  {
    void onTabSelected(@NonNull Tab tab);
  }

  private class PageChangedListener extends TabLayout.TabLayoutOnPageChangeListener
  {
    PageChangedListener(TabLayout tabs)
    {
      super(tabs);
    }

    @Override
    public void onPageSelected(int position)
    {
      super.onPageSelected(position);
      if (mTabSelectedListener != null)
        mTabSelectedListener.onTabSelected(Tab.values()[position]);
    }
  }

  private static class OnTabSelectedListenerForViewPager extends TabLayout.ViewPagerOnTabSelectedListener
  {
    @NonNull
    private final Context mContext;

    OnTabSelectedListenerForViewPager(ViewPager viewPager)
    {
      super(viewPager);
      mContext = viewPager.getContext();
    }

    @Override
    public void onTabSelected(TabLayout.Tab tab)
    {
      super.onTabSelected(tab);
      Graphics.tint(mContext, tab.getIcon(), R.attr.colorAccent);
    }

    @Override
    public void onTabUnselected(TabLayout.Tab tab)
    {
      super.onTabUnselected(tab);
      Graphics.tint(mContext, tab.getIcon());
    }
  }

  private final ViewPager mPager;
  private final List<Class<? extends Fragment>> mClasses = new ArrayList<>();
  private final SparseArray<Fragment> mFragments = new SparseArray<>();
  private OnTabSelectedListener mTabSelectedListener;

  TabAdapter(FragmentManager fragmentManager, ViewPager pager, TabLayout tabs)
  {
    super(fragmentManager);

    for (Tab tab : Tab.values())
      mClasses.add(tab.getFragmentClass());

    final List<Fragment> fragments = fragmentManager.getFragments();
    if (fragments != null)
    {
      // Recollect already attached fragments
      for (Fragment f : fragments)
      {
        if (f == null)
          continue;

        final int idx = mClasses.indexOf(f.getClass());
        if (idx > -1)
          mFragments.put(idx, f);
      }
    }

    mPager = pager;
    mPager.setAdapter(this);

    attachTo(tabs);
  }

  private static ColorStateList getTabTextColor(Context context)
  {
    return context.getResources().getColorStateList(ThemeUtils.isNightTheme() ? R.color.accent_color_selector_night
                                                                              : R.color.accent_color_selector);
  }

  private void attachTo(TabLayout tabs)
  {
    final Context context = tabs.getContext();

    for (Tab tab : Tab.values())
    {
      TabLayout.Tab t = tabs.newTab();
      t.setText(tab.getTitleRes());
      tabs.addTab(t, false);
    }

    ViewPager.OnPageChangeListener listener = new PageChangedListener(tabs);
    mPager.addOnPageChangeListener(listener);
    tabs.setOnTabSelectedListener(new OnTabSelectedListenerForViewPager(mPager));
    listener.onPageSelected(0);
  }

  void setTabSelectedListener(OnTabSelectedListener listener)
  {
    mTabSelectedListener = listener;
  }

  @Override
  public Fragment getItem(int position)
  {
    Fragment res = mFragments.get(position);
    if (res == null || res.getClass() != mClasses.get(position))
    {
      //noinspection TryWithIdenticalCatches
      try
      {
        res = mClasses.get(position).newInstance();
        mFragments.put(position, res);
      } catch (InstantiationException ignored)
      {}
      catch (IllegalAccessException ignored)
      {}
    }

    return res;
  }

  @Override
  public int getCount()
  {
    return mClasses.size();
  }
}
