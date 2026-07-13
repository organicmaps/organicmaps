package app.organicmaps.search;

import android.content.Context;
import android.content.SharedPreferences;
import android.util.SparseArray;
import androidx.annotation.NonNull;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentManager;
import androidx.fragment.app.FragmentPagerAdapter;
import androidx.fragment.app.FragmentTransaction;
import androidx.viewpager.widget.ViewPager;
import app.organicmaps.MwmApplication;
import app.organicmaps.R;
import app.organicmaps.sdk.util.Config;
import app.organicmaps.util.Graphics;
import com.google.android.material.tabs.TabLayout;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

class TabAdapter extends FragmentPagerAdapter
{
  enum Tab
  {
    HISTORY {
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

    CATEGORIES {
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
      SharedPreferences.Editor editor = MwmApplication.prefs(mContext).edit();
      editor.putInt(Config.KEY_PREF_LAST_SEARCHED_TAB, tab.getPosition());
      editor.apply();
      super.onTabSelected(tab);
      Graphics.tint(mContext, tab.getIcon(), androidx.appcompat.R.attr.colorAccent);
    }

    @Override
    public void onTabUnselected(TabLayout.Tab tab)
    {
      super.onTabUnselected(tab);
      Graphics.tint(mContext, tab.getIcon());
    }
  }

  private final FragmentManager mFragmentManager;
  private final ViewPager mPager;
  private final List<Class<? extends Fragment>> mClasses = new ArrayList<>();
  private final SparseArray<Fragment> mFragments = new SparseArray<>();
  private OnTabSelectedListener mTabSelectedListener;
  private final TabLayout mTabs;
  private final boolean mHistoryEnabled;
  TabAdapter(FragmentManager fragmentManager, ViewPager pager, TabLayout tabs, boolean historyEnabled)
  {
    super(fragmentManager);
    this.mFragmentManager = fragmentManager;
    this.mTabs = tabs;
    this.mHistoryEnabled = historyEnabled;
    for (Tab tab : Tab.values())
    {
      if (tab == Tab.HISTORY && !historyEnabled)
        continue;
      mClasses.add(tab.getFragmentClass());
    }
    final List<Fragment> tabFragments = new ArrayList<>();
    final Set<Class<? extends Fragment>> restoredClasses = new HashSet<>();
    for (Fragment f : fragmentManager.getFragments())
    {
      if (f == null || !isTabFragment(f))
        continue;
      tabFragments.add(f);
      restoredClasses.add(f.getClass());
    }

    if (restoredClasses.equals(new HashSet<>(mClasses)))
    {
      // Recollect already attached fragments
      for (Fragment f : tabFragments)
        mFragments.put(mClasses.indexOf(f.getClass()), f);
    }
    else
      removeFragments(tabFragments);

    mPager = pager;
    mPager.setAdapter(this);
    if (mHistoryEnabled)
      attachTo(tabs);
  }

  boolean isHistoryEnabled()
  {
    return mHistoryEnabled;
  }

  private void removeFragments(@NonNull List<Fragment> fragments)
  {
    if (fragments.isEmpty())
      return;

    final FragmentTransaction tx = mFragmentManager.beginTransaction();
    for (Fragment f : fragments)
      tx.remove(f);
    tx.commitNowAllowingStateLoss();
  }

  private static boolean isTabFragment(@NonNull Fragment f)
  {
    for (Tab tab : Tab.values())
    {
      if (tab.getFragmentClass() == f.getClass())
        return true;
    }
    return false;
  }

  private void attachTo(TabLayout tabs)
  {
    for (Tab tab : Tab.values())
    {
      TabLayout.Tab t = tabs.newTab();
      t.setText(tab.getTitleRes());
      tabs.addTab(t, false);
    }

    ViewPager.OnPageChangeListener listener = new PageChangedListener(tabs);
    mPager.addOnPageChangeListener(listener);
    tabs.setOnTabSelectedListener(new OnTabSelectedListenerForViewPager(mPager));
    SharedPreferences preferences = MwmApplication.prefs(mPager.getContext());
    int lastSelectedTabPosition = preferences.getInt(Config.KEY_PREF_LAST_SEARCHED_TAB, 0);
    listener.onPageSelected(lastSelectedTabPosition);
  }

  void setTabSelectedListener(OnTabSelectedListener listener)
  {
    mTabSelectedListener = listener;
  }

  void destroy()
  {
    mPager.setAdapter(null);
    final List<Fragment> fragments = new ArrayList<>();
    for (int i = 0; i < mFragments.size(); i++)
      fragments.add(mFragments.valueAt(i));
    mFragments.clear();
    removeFragments(fragments);
    mTabs.removeAllTabs();
  }

  @Override
  public Fragment getItem(int position)
  {
    Fragment res = mFragments.get(position);
    if (res == null || res.getClass() != mClasses.get(position))
    {
      // noinspection TryWithIdenticalCatches
      try
      {
        res = mClasses.get(position).newInstance();
        mFragments.put(position, res);
      }
      catch (InstantiationException ignored)
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
