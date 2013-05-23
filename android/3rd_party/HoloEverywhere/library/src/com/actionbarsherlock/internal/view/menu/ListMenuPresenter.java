
package com.actionbarsherlock.internal.view.menu;

import java.util.ArrayList;

import org.holoeverywhere.LayoutInflater;
import org.holoeverywhere.R;
import org.holoeverywhere.app.ContextThemeWrapperPlus;

import android.content.Context;
import android.os.Bundle;
import android.os.Parcelable;
import android.util.SparseArray;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.ListAdapter;

public class ListMenuPresenter implements MenuPresenter,
        AdapterView.OnItemClickListener {
    private class MenuAdapter extends BaseAdapter {
        private int mExpandedIndex = -1;

        public MenuAdapter() {
            findExpandedIndex();
        }

        void findExpandedIndex() {
            final MenuItemImpl expandedItem = mMenu.getExpandedItem();
            if (expandedItem != null) {
                final ArrayList<MenuItemImpl> items = mMenu.getNonActionItems();
                final int count = items.size();
                for (int i = 0; i < count; i++) {
                    final MenuItemImpl item = items.get(i);
                    if (item == expandedItem) {
                        mExpandedIndex = i;
                        return;
                    }
                }
            }
            mExpandedIndex = -1;
        }

        @Override
        public int getCount() {
            ArrayList<MenuItemImpl> items = mMenu.getNonActionItems();
            int count = items.size() - mItemIndexOffset;
            if (mExpandedIndex < 0) {
                return count;
            }
            return count - 1;
        }

        @Override
        public MenuItemImpl getItem(int position) {
            ArrayList<MenuItemImpl> items = mMenu.getNonActionItems();
            position += mItemIndexOffset;
            if (mExpandedIndex >= 0 && position >= mExpandedIndex) {
                position++;
            }
            return items.get(position);
        }

        @Override
        public long getItemId(int position) {
            return position;
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            if (convertView == null) {
                convertView = mInflater.inflate(mItemLayoutRes, parent, false);
            }

            MenuView.ItemView itemView = (MenuView.ItemView) convertView;
            itemView.initialize(getItem(position), 0);
            return convertView;
        }

        @Override
        public void notifyDataSetChanged() {
            findExpandedIndex();
            super.notifyDataSetChanged();
        }
    }

    public static final String VIEWS_TAG = "android:menu:list";
    MenuAdapter mAdapter;
    private Callback mCallback;
    Context mContext;
    private int mId;
    LayoutInflater mInflater;
    private int mItemIndexOffset;
    int mItemLayoutRes;
    MenuBuilder mMenu;
    ExpandedMenuView mMenuView;

    int mThemeRes;

    public ListMenuPresenter(Context context, int itemLayoutRes) {
        this(itemLayoutRes, 0);
        mContext = context;
        mInflater = LayoutInflater.from(mContext);
    }

    public ListMenuPresenter(int itemLayoutRes, int themeRes) {
        mItemLayoutRes = itemLayoutRes;
        mThemeRes = themeRes;
    }

    @Override
    public boolean collapseItemActionView(MenuBuilder menu, MenuItemImpl item) {
        return false;
    }

    @Override
    public boolean expandItemActionView(MenuBuilder menu, MenuItemImpl item) {
        return false;
    }

    @Override
    public boolean flagActionItems() {
        return false;
    }

    public ListAdapter getAdapter() {
        if (mAdapter == null) {
            mAdapter = new MenuAdapter();
        }
        return mAdapter;
    }

    @Override
    public int getId() {
        return mId;
    }

    int getItemIndexOffset() {
        return mItemIndexOffset;
    }

    @Override
    public MenuView getMenuView(ViewGroup root) {
        if (mMenuView == null) {
            mMenuView = (ExpandedMenuView) mInflater.inflate(
                    R.layout.expanded_menu_layout, root, false);
            if (mAdapter == null) {
                mAdapter = new MenuAdapter();
            }
            mMenuView.setAdapter(mAdapter);
            mMenuView.setOnItemClickListener(this);
        }
        return mMenuView;
    }

    @Override
    public void initForMenu(Context context, MenuBuilder menu) {
        if (mThemeRes != 0) {
            mContext = new ContextThemeWrapperPlus(context, mThemeRes);
            mInflater = LayoutInflater.from(mContext);
        } else if (mContext != null) {
            mContext = context;
            if (mInflater == null) {
                mInflater = LayoutInflater.from(mContext);
            }
        }
        mMenu = menu;
        if (mAdapter != null) {
            mAdapter.notifyDataSetChanged();
        }
    }

    @Override
    public void onCloseMenu(MenuBuilder menu, boolean allMenusAreClosing) {
        if (mCallback != null) {
            mCallback.onCloseMenu(menu, allMenusAreClosing);
        }
    }

    @Override
    public void onItemClick(AdapterView<?> parent, View view, int position,
            long id) {
        mMenu.performItemAction(mAdapter.getItem(position), 0);
    }

    @Override
    public void onRestoreInstanceState(Parcelable state) {
        restoreHierarchyState((Bundle) state);
    }

    @Override
    public Parcelable onSaveInstanceState() {
        if (mMenuView == null) {
            return null;
        }

        Bundle state = new Bundle();
        saveHierarchyState(state);
        return state;
    }

    @Override
    public boolean onSubMenuSelected(SubMenuBuilder subMenu) {
        if (!subMenu.hasVisibleItems()) {
            return false;
        }
        new MenuDialogHelper(subMenu).show(null);
        if (mCallback != null) {
            mCallback.onOpenSubMenu(subMenu);
        }
        return true;
    }

    public void restoreHierarchyState(Bundle inState) {
        SparseArray<Parcelable> viewStates = inState
                .getSparseParcelableArray(ListMenuPresenter.VIEWS_TAG);
        if (viewStates != null) {
            ((View) mMenuView).restoreHierarchyState(viewStates);
        }
    }

    public void saveHierarchyState(Bundle outState) {
        SparseArray<Parcelable> viewStates = new SparseArray<Parcelable>();
        if (mMenuView != null) {
            ((View) mMenuView).saveHierarchyState(viewStates);
        }
        outState.putSparseParcelableArray(ListMenuPresenter.VIEWS_TAG,
                viewStates);
    }

    @Override
    public void setCallback(Callback cb) {
        mCallback = cb;
    }

    public void setId(int id) {
        mId = id;
    }

    public void setItemIndexOffset(int offset) {
        mItemIndexOffset = offset;
        if (mMenuView != null) {
            updateMenuView(false);
        }
    }

    @Override
    public void updateMenuView(boolean cleared) {
        if (mAdapter != null) {
            mAdapter.notifyDataSetChanged();
        }
    }
}
