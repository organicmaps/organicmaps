package com.mapswithme.maps.search;

import android.content.Context;
import android.content.res.ColorStateList;
import android.content.res.Configuration;
import android.support.design.widget.TabLayout;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentPagerAdapter;
import android.support.v4.view.ViewPager;
import android.util.SparseArray;
import android.view.LayoutInflater;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.List;

import com.mapswithme.maps.R;
import com.mapswithme.util.Graphics;
import com.mapswithme.util.ThemeUtils;
import com.mapswithme.util.UiUtils;

class TabAdapter extends FragmentPagerAdapter
{
  enum Tab
  {
    HISTORY
    {
      @Override
      public int getIconRes()
      {
        return R.drawable.ic_search_tab_history;
      }

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
      public int getIconRes()
      {
        return R.drawable.ic_search_tab_categories;
      }

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

    public abstract int getIconRes();
    public abstract int getTitleRes();
    public abstract Class<? extends Fragment> getFragmentClass();
  }

  interface OnTabSelectedListener
  {
    void onTabSelected(Tab tab);
  }

  // Workaround for https://code.google.com/p/android/issues/detail?id=180454
  // TODO: Remove this class and replace with TabLayout.TabLayoutOnPageChangeListener after library fix
  private class CustomTabLayoutOnPageChangeListener implements ViewPager.OnPageChangeListener
  {
    private final TabLayout mTabs;
    private int mScrollState;
    private int mPreviousScrollState;

    public CustomTabLayoutOnPageChangeListener(TabLayout tabs)
    {
      mTabs = tabs;
    }

    @Override
    public void onPageScrollStateChanged(int state)
    {
      mPreviousScrollState = mScrollState;
      mScrollState = state;
    }

    @Override
    public void onPageScrolled(int position, float positionOffset, int positionOffsetPixels)
    {
      final boolean update = (mScrollState == ViewPager.SCROLL_STATE_DRAGGING ||
                              (mScrollState == ViewPager.SCROLL_STATE_SETTLING &&
                               mPreviousScrollState == ViewPager.SCROLL_STATE_DRAGGING));
      mTabs.setScrollPosition(position, positionOffset, update);
    }

    @Override
    public void onPageSelected(int position)
    {
      mTabs.getTabAt(position).select();
      if (mTabSelectedListener != null)
        mTabSelectedListener.onTabSelected(TABS[position]);
    }
  }

  public static final Tab[] TABS = Tab.values();

  private final ViewPager mPager;
  private final List<Class<? extends Fragment>> mClasses = new ArrayList<>();
  private final SparseArray<Fragment> mFragments = new SparseArray<>();
  private OnTabSelectedListener mTabSelectedListener;

  public TabAdapter(FragmentManager fragmentManager, ViewPager pager, TabLayout tabs)
  {
    super(fragmentManager);

    for (Tab tab : TABS)
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
    return context.getResources().getColorStateList(ThemeUtils.isNightTheme() ? R.color.tab_text_night
                                                                              : R.color.tab_text);
  }

  private void attachTo(TabLayout tabs)
  {
    final Context context = tabs.getContext();
    final LayoutInflater inflater = LayoutInflater.from(context);
    boolean landscape = (context.getResources().getConfiguration().orientation == Configuration.ORIENTATION_LANDSCAPE);

    int padding = UiUtils.dimen(landscape ? R.dimen.margin_half
                                          : R.dimen.margin_eighth);
    for (Tab tab : TABS)
    {
      final TextView tabView = (TextView) inflater.inflate(R.layout.tab, tabs, false);
      tabView.setText(tab.getTitleRes());
      tabView.setCompoundDrawablePadding(padding);
      tabView.setCompoundDrawablesWithIntrinsicBounds(landscape ? tab.getIconRes() : 0, landscape ? 0 : tab.getIconRes(), 0, 0);

      ColorStateList colors = getTabTextColor(context);
      tabView.setTextColor(colors);
      Graphics.tint(tabView, colors);

      tabs.addTab(tabs.newTab().setCustomView(tabView), true);
    }

    ViewPager.OnPageChangeListener listener = new CustomTabLayoutOnPageChangeListener(tabs);
    mPager.addOnPageChangeListener(listener);
    tabs.setOnTabSelectedListener(new TabLayout.ViewPagerOnTabSelectedListener(mPager));
    listener.onPageSelected(0);
  }

  public void setTabSelectedListener(OnTabSelectedListener listener)
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
