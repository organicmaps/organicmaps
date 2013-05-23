
package org.holoeverywhere.app;

import org.holoeverywhere.ITabSwipe;
import org.holoeverywhere.R;
import org.holoeverywhere.widget.FrameLayout;

import android.os.Bundle;

/**
 * This activity class implement tabs + swipe navigation pattern<br />
 * <br />
 * Part of HoloEverywhere
 */
public abstract class TabSwipeActivity extends Activity implements
        ITabSwipe<TabSwipeFragment.TabInfo> {
    public static class InnerFragment extends TabSwipeFragment {
        private TabSwipeActivity mActivity;
        private boolean mTabsWasHandled = false;

        @Override
        protected void onHandleTabs() {
            mTabsWasHandled = true;
            if (mActivity != null) {
                mActivity.onHandleTabs();
            }
        }

        public void setActivity(TabSwipeActivity activity) {
            if (activity == null) {
                return;
            }
            mActivity = activity;
            if (mTabsWasHandled) {
                mActivity.onHandleTabs();
            }
        }
    }

    private int mCustomLayout = -1;

    private InnerFragment mFragment;

    @Override
    public TabSwipeFragment.TabInfo addTab(CharSequence title,
            Class<? extends Fragment> fragmentClass) {
        return mFragment.addTab(title, fragmentClass);
    }

    @Override
    public TabSwipeFragment.TabInfo addTab(CharSequence title,
            Class<? extends Fragment> fragmentClass, Bundle fragmentArguments) {
        return mFragment.addTab(title, fragmentClass, fragmentArguments);
    }

    @Override
    public TabSwipeFragment.TabInfo addTab(int title,
            Class<? extends Fragment> fragmentClass) {
        return mFragment.addTab(title, fragmentClass);
    }

    @Override
    public TabSwipeFragment.TabInfo addTab(int title,
            Class<? extends Fragment> fragmentClass, Bundle fragmentArguments) {
        return mFragment.addTab(title, fragmentClass, fragmentArguments);
    }

    @Override
    public TabSwipeFragment.TabInfo addTab(
            TabSwipeFragment.TabInfo tabInfo) {
        return mFragment.addTab(tabInfo);
    }

    @Override
    public TabSwipeFragment.TabInfo addTab(
            TabSwipeFragment.TabInfo tabInfo, int position) {
        return mFragment.addTab(tabInfo, position);
    }

    @Override
    public OnTabSelectedListener getOnTabSelectedListener() {
        return mFragment.getOnTabSelectedListener();
    }

    @Override
    public boolean isSmoothScroll() {
        return mFragment.isSmoothScroll();
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mFragment = (InnerFragment) getSupportFragmentManager().findFragmentById(R.id.contentPanel);
        if (mFragment == null) {
            mFragment = new InnerFragment();
        }
        mFragment.setActivity(this);
        if (mCustomLayout > 0) {
            mFragment.setCustomLayout(mCustomLayout);
        }
        if (mFragment.isDetached()) {
            FrameLayout layout = new FrameLayout(this);
            layout.setId(R.id.contentPanel);
            setContentView(layout);
            getSupportFragmentManager().beginTransaction()
                    .replace(R.id.contentPanel, mFragment).commit();
            getSupportFragmentManager().executePendingTransactions();
        }
    }

    /**
     * Add your tabs here
     */
    protected abstract void onHandleTabs();

    @Override
    public void reloadTabs() {
        mFragment.reloadTabs();
    }

    @Override
    public void removeAllTabs() {
        mFragment.removeAllTabs();
    }

    @Override
    public TabSwipeFragment.TabInfo removeTab(int position) {
        return mFragment.removeTab(position);
    }

    @Override
    public TabSwipeFragment.TabInfo removeTab(
            TabSwipeFragment.TabInfo tabInfo) {
        return mFragment.removeTab(tabInfo);
    }

    @Override
    public void setCurrentTab(int position) {
        mFragment.setCurrentTab(position);
    }

    @Override
    public void setCustomLayout(int customLayout) {
        mCustomLayout = customLayout;
    }

    @Override
    public void setOnTabSelectedListener(OnTabSelectedListener onTabSelectedListener) {
        mFragment.setOnTabSelectedListener(onTabSelectedListener);
    }

    @Override
    public void setSmoothScroll(boolean smoothScroll) {
        mFragment.setSmoothScroll(smoothScroll);
    }
}
