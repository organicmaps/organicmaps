package com.mapswithme.maps.search;

import android.content.Context;
import android.content.res.Configuration;
import android.support.design.widget.TabLayout;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentPagerAdapter;
import android.support.v4.view.ViewPager;
import android.util.SparseArray;
import android.view.LayoutInflater;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.util.Graphics;

import java.util.ArrayList;
import java.util.List;

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

  // Workaround for https://code.google.com/p/android/issues/detail?id=180454
  // TODO: Remove this class and replace with TabLayout.TabLayoutOnPageChangeListener after library fix
  private static class CustomTabLayoutOnPageChangeListener implements ViewPager.OnPageChangeListener
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
      boolean update = (mScrollState == ViewPager.SCROLL_STATE_DRAGGING ||
                        (mScrollState == ViewPager.SCROLL_STATE_SETTLING &&
                         mPreviousScrollState == ViewPager.SCROLL_STATE_DRAGGING));
      mTabs.setScrollPosition(position, positionOffset, update);
    }

    @Override
    public void onPageSelected(int position)
    {
      mTabs.getTabAt(position).select();
    }
  }


  public static final Tab[] TABS = Tab.values();

  private final ViewPager mPager;
  private final List<Class<? extends Fragment>> mClasses = new ArrayList<>();
  private final SparseArray<Fragment> mFragments = new SparseArray<>();


  public TabAdapter(FragmentManager fragmentManager, ViewPager pager, TabLayout tabs)
  {
    super(fragmentManager);

    for (Tab tab : TABS)
      mClasses.add(tab.getFragmentClass());

    List<Fragment> fragments = fragmentManager.getFragments();
    if (fragments != null)
    {
      // Recollect already attached fragments
      for (Fragment f : fragments)
      {
        if (f == null)
          continue;

        int idx = mClasses.indexOf(f.getClass());
        if (idx > -1)
          mFragments.put(idx, f);
      }
    }

    mPager = pager;
    mPager.setAdapter(this);

    attachTo(tabs);
  }

  private void attachTo(TabLayout tabs)
  {
    Context context = tabs.getContext();
    LayoutInflater inflater = LayoutInflater.from(context);
    boolean landscape = (context.getResources().getConfiguration().orientation == Configuration.ORIENTATION_LANDSCAPE);

    for (Tab tab : TABS)
    {
      TextView tabView = (TextView) inflater.inflate(R.layout.tab, tabs, false);
      tabView.setText(tab.getTitleRes());
      tabView.setCompoundDrawablesWithIntrinsicBounds(landscape ? tab.getIconRes() : 0, landscape ? 0 : tab.getIconRes(), 0, 0);
      Graphics.tintTextView(tabView, context.getResources().getColorStateList(R.color.text_highlight));

      tabs.addTab(tabs.newTab().setCustomView(tabView), true);
    }

    ViewPager.OnPageChangeListener listener = new CustomTabLayoutOnPageChangeListener(tabs);
    mPager.addOnPageChangeListener(listener);
    tabs.setOnTabSelectedListener(new TabLayout.ViewPagerOnTabSelectedListener(mPager));
    listener.onPageSelected(0);
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

  public Tab getCurrentTab()
  {
    return TABS[mPager.getCurrentItem()];
  }
}
