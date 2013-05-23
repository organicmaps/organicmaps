
package org.holoeverywhere.app;

import java.util.ArrayList;
import java.util.List;

import org.holoeverywhere.ITabSwipe;
import org.holoeverywhere.LayoutInflater;
import org.holoeverywhere.R;
import org.holoeverywhere.app.TabSwipeFragment.TabInfo;

import android.os.Bundle;
import android.support.v4.app.FragmentStatePagerAdapter;
import android.support.v4.app.FragmentTransaction;
import android.support.v4.view.ViewPager;
import android.support.v4.view.ViewPager.OnPageChangeListener;
import android.view.View;
import android.view.ViewGroup;

import com.actionbarsherlock.app.ActionBar;
import com.actionbarsherlock.app.ActionBar.Tab;
import com.actionbarsherlock.app.ActionBar.TabListener;

/**
 * This fragment class implement tabs + swipe navigation pattern<br />
 * <br />
 * Part of HoloEverywhere
 */
public abstract class TabSwipeFragment extends Fragment implements ITabSwipe<TabInfo> {
    public static class TabInfo implements ITabSwipe.TabInfo {
        public Bundle fragmentArguments;
        public Class<? extends Fragment> fragmentClass;
        public CharSequence title;

        @Override
        public Bundle getFragmentArguments() {
            return fragmentArguments;
        }

        @Override
        public Class<? extends Fragment> getFragmentClass() {
            return fragmentClass;
        }

        @Override
        public CharSequence getTitle() {
            return title;
        }

        @Override
        public void setFragmentArguments(Bundle fragmentArguments) {
            this.fragmentArguments = fragmentArguments;
        }

        @Override
        public void setFragmentClass(Class<? extends Fragment> fragmentClass) {
            this.fragmentClass = fragmentClass;
        }

        @Override
        public void setTitle(CharSequence title) {
            this.title = title;
        }
    }

    private final class TabSwipeAdapter extends FragmentStatePagerAdapter implements
            OnPageChangeListener, TabListener {
        public TabSwipeAdapter() {
            super(getChildFragmentManager());
        }

        @Override
        public int getCount() {
            return mTabs.size();
        }

        @Override
        public Fragment getItem(int position) {
            final TabInfo info = mTabs.get(position);
            return Fragment.instantiate(info.fragmentClass, info.fragmentArguments);
        }

        @Override
        public void onPageScrolled(int position, float percent, int pixels) {
            // Do nothing
        }

        @Override
        public void onPageScrollStateChanged(int scrollState) {
            // Do nothing
        }

        @Override
        public void onPageSelected(int position) {
            dispatchTabSelected(position);
        }

        @Override
        public void onTabReselected(Tab tab, FragmentTransaction ft) {
            // Do nothing
        }

        @Override
        public void onTabSelected(Tab tab, FragmentTransaction ft) {
            dispatchTabSelected(tab.getPosition());
        }

        @Override
        public void onTabUnselected(Tab tab, FragmentTransaction ft) {
            // Do nothing
        }
    }

    private TabSwipeAdapter mAdapter;
    private int mCustomLayout = -1;
    private OnTabSelectedListener mOnTabSelectedListener;
    private int mPrevNavigationMode = ActionBar.NAVIGATION_MODE_STANDARD;
    private boolean mSmoothScroll = true;
    private List<TabInfo> mTabs = new ArrayList<TabInfo>();

    private ViewPager mViewPager;

    @Override
    public TabInfo addTab(CharSequence title, Class<? extends Fragment> fragmentClass) {
        return addTab(title, fragmentClass, null);
    }

    @Override
    public TabInfo addTab(CharSequence title, Class<? extends Fragment> fragmentClass,
            Bundle fragmentArguments) {
        TabInfo info = new TabInfo();
        info.title = title;
        info.fragmentClass = fragmentClass;
        info.fragmentArguments = fragmentArguments;
        return addTab(info);
    }

    @Override
    public TabInfo addTab(int title, Class<? extends Fragment> fragmentClass) {
        return addTab(getText(title), fragmentClass, null);
    }

    @Override
    public TabInfo addTab(int title, Class<? extends Fragment> fragmentClass,
            Bundle fragmentArguments) {
        return addTab(getText(title), fragmentClass, fragmentArguments);
    }

    @Override
    public TabInfo addTab(TabInfo tabInfo) {
        mTabs.add(tabInfo);
        getSupportActionBar().addTab(makeActionBarTab(tabInfo));
        notifyChanged();
        return tabInfo;
    }

    @Override
    public TabInfo addTab(TabInfo tabInfo, int position) {
        mTabs.add(position, tabInfo);
        getSupportActionBar().addTab(makeActionBarTab(tabInfo), position);
        notifyChanged();
        return tabInfo;
    }

    private void dispatchTabSelected(int position) {
        boolean notify = false;
        if (mViewPager.getCurrentItem() != position) {
            mViewPager.setCurrentItem(position, mSmoothScroll);
            notify = true;
        }
        if (getSupportActionBar().getSelectedNavigationIndex() != position) {
            getSupportActionBar().selectTab(getSupportActionBar().getTabAt(position));
            notify = true;
        }
        if (notify) {
            onTabSelected(position);
        }
    }

    @Override
    public OnTabSelectedListener getOnTabSelectedListener() {
        return mOnTabSelectedListener;
    }

    @Override
    public boolean isSmoothScroll() {
        return mSmoothScroll;
    }

    protected Tab makeActionBarTab(TabInfo tabInfo) {
        Tab tab = getSupportActionBar().newTab();
        tab.setText(tabInfo.title);
        tab.setTabListener(mAdapter);
        return tab;
    }

    private void notifyChanged() {
        if (mAdapter != null) {
            mAdapter.notifyDataSetChanged();
        }
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        return inflater.inflate(mCustomLayout > 0 ? mCustomLayout : R.layout.tab_swipe, container,
                false);
    }

    @Override
    public void onDestroyView() {
        getSupportActionBar().removeAllTabs();
        getSupportActionBar().setNavigationMode(mPrevNavigationMode);
        super.onDestroyView();
    }

    /**
     * Add your tabs here
     */
    protected abstract void onHandleTabs();

    public void onTabSelected(int position) {
        if (mOnTabSelectedListener != null) {
            mOnTabSelectedListener.onTabSelected(position);
        }
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        mViewPager = (ViewPager) view.findViewById(R.id.tabSwipePager);
        if (mViewPager == null) {
            throw new IllegalStateException(
                    "Add ViewPager to your custom layout with id @id/tabSwipePager");
        }
        if (getSupportActionBar().getTabCount() > 0) {
            throw new IllegalStateException(
                    "TabSwipeFragment doesn't support multitabbed fragments");
        }
        mAdapter = new TabSwipeAdapter();
        onHandleTabs();
        mViewPager.setAdapter(mAdapter);
        mViewPager.setOnPageChangeListener(mAdapter);
        mPrevNavigationMode = getSupportActionBar().getNavigationMode();
        getSupportActionBar().setNavigationMode(ActionBar.NAVIGATION_MODE_TABS);
    }

    @Override
    public void reloadTabs() {
        removeAllTabs();
        onHandleTabs();
    }

    @Override
    public void removeAllTabs() {
        getSupportActionBar().removeAllTabs();
        mTabs.clear();
        notifyChanged();
    }

    @Override
    public TabInfo removeTab(int position) {
        TabInfo tabInfo = mTabs.remove(position);
        getSupportActionBar().removeTabAt(position);
        notifyChanged();
        return tabInfo;
    }

    @Override
    public TabInfo removeTab(TabInfo tabInfo) {
        for (int i = 0; i < mTabs.size(); i++) {
            if (mTabs.get(i) == tabInfo) {
                return removeTab(i);
            }
        }
        return tabInfo;
    }

    @Override
    public void setCurrentTab(int position) {
        dispatchTabSelected(Math.min(0, Math.max(position, mTabs.size() - 1)));
    }

    /**
     * If you want custom layout for this activity - call this method before
     * super.onCreate<br />
     * Your layout should be contains ViewPager with id @id/tabSwipePager
     */
    @Override
    public void setCustomLayout(int customLayout) {
        mCustomLayout = customLayout;
    }

    @Override
    public void setOnTabSelectedListener(OnTabSelectedListener onTabSelectedListener) {
        mOnTabSelectedListener = onTabSelectedListener;
    }

    /**
     * Smooth scroll of ViewPager when user click on tab
     */
    @Override
    public void setSmoothScroll(boolean smoothScroll) {
        mSmoothScroll = smoothScroll;
    }
}
