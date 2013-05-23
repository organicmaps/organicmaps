
package com.actionbarsherlock.internal.view.menu;

import java.util.ArrayList;

import org.holoeverywhere.LayoutInflater;
import org.holoeverywhere.R;
import org.holoeverywhere.widget.FrameLayout;
import org.holoeverywhere.widget.ListPopupWindow;
import org.holoeverywhere.widget.PopupWindow;

import android.content.Context;
import android.content.res.Resources;
import android.database.DataSetObserver;
import android.os.Parcelable;
import android.view.KeyEvent;
import android.view.View;
import android.view.View.MeasureSpec;
import android.view.ViewGroup;
import android.view.ViewTreeObserver;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.ListAdapter;

import com.actionbarsherlock.internal.view.View_HasStateListenerSupport;
import com.actionbarsherlock.internal.view.View_OnAttachStateChangeListener;
import com.actionbarsherlock.view.MenuItem;

public class PopupMenuHelper implements AdapterView.OnItemClickListener,
        View.OnKeyListener, ViewTreeObserver.OnGlobalLayoutListener,
        PopupWindow.OnDismissListener, View_OnAttachStateChangeListener,
        MenuPresenter {
    private class ExpandedIndexObserver extends DataSetObserver {
        @Override
        public void onChanged() {
            mAdapter.findExpandedIndex();
        }
    }

    private class MenuAdapter extends BaseAdapter {
        private MenuBuilder mAdapterMenu;
        private int mExpandedIndex = -1;

        public MenuAdapter(MenuBuilder menu) {
            mAdapterMenu = menu;
            registerDataSetObserver(new ExpandedIndexObserver());
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
            ArrayList<MenuItemImpl> items = mAdapterMenu.getVisibleItems();
            if (mExpandedIndex < 0) {
                return items.size();
            }
            return items.size() - 1;
        }

        @Override
        public MenuItemImpl getItem(int position) {
            ArrayList<MenuItemImpl> items = mAdapterMenu.getVisibleItems();
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
                convertView = mInflater.inflate(ITEM_LAYOUT, parent, false);
            }
            MenuView.ItemView itemView = (MenuView.ItemView) convertView;
            if (mForceShowIcon) {
                ((HoloListMenuItemView) convertView).setForceShowIcon(true);
            }
            itemView.initialize(getItem(position), 0);
            return convertView;
        }
    }

    static final int ITEM_LAYOUT = R.layout.popup_menu_item_layout;
    private MenuAdapter mAdapter;
    private View mAnchorView;
    private Context mContext;
    boolean mForceShowIcon;
    private LayoutInflater mInflater;
    private ViewGroup mMeasureParent;
    private MenuBuilder mMenu;
    private ListPopupWindow mPopup;
    private int mPopupMaxWidth;
    private Callback mPresenterCallback;
    private ViewTreeObserver mTreeObserver;

    public PopupMenuHelper(Context context, MenuBuilder menu, View decorView) {
        mContext = context;
        mInflater = LayoutInflater.from(context);
        mMenu = menu;
        final Resources res = context.getResources();
        mPopupMaxWidth = Math.max(res.getDisplayMetrics().widthPixels / 2,
                res.getDimensionPixelSize(R.dimen.config_prefDialogWidth));
        mAnchorView = decorView;
        menu.addMenuPresenter(this);
    }

    @Override
    public boolean collapseItemActionView(MenuBuilder menu, MenuItemImpl item) {
        return false;
    }

    public void dismiss() {
        if (isShowing()) {
            mPopup.dismiss();
        }
    }

    @Override
    public boolean expandItemActionView(MenuBuilder menu, MenuItemImpl item) {
        return false;
    }

    @Override
    public boolean flagActionItems() {
        return false;
    }

    @Override
    public int getId() {
        return 0;
    }

    @Override
    public MenuView getMenuView(ViewGroup root) {
        throw new UnsupportedOperationException(
                "MenuPopupHelpers manage their own views");
    }

    @Override
    public void initForMenu(Context context, MenuBuilder menu) {
    }

    public boolean isShowing() {
        return mPopup != null && mPopup.isShowing();
    }

    private int measureContentWidth(ListAdapter adapter) {
        int width = 0;
        View itemView = null;
        int itemType = 0;
        final int widthMeasureSpec = MeasureSpec.makeMeasureSpec(0,
                MeasureSpec.UNSPECIFIED);
        final int heightMeasureSpec = MeasureSpec.makeMeasureSpec(0,
                MeasureSpec.UNSPECIFIED);
        final int count = adapter.getCount();
        for (int i = 0; i < count; i++) {
            final int positionType = adapter.getItemViewType(i);
            if (positionType != itemType) {
                itemType = positionType;
                itemView = null;
            }
            if (mMeasureParent == null) {
                mMeasureParent = new FrameLayout(mContext);
            }
            itemView = adapter.getView(i, itemView, mMeasureParent);
            itemView.measure(widthMeasureSpec, heightMeasureSpec);
            width = Math.max(width, itemView.getMeasuredWidth());
        }
        return width;
    }

    @Override
    public void onCloseMenu(MenuBuilder menu, boolean allMenusAreClosing) {
        if (menu != mMenu) {
            return;
        }
        dismiss();
        if (mPresenterCallback != null) {
            mPresenterCallback.onCloseMenu(menu, allMenusAreClosing);
        }
    }

    @Override
    @SuppressWarnings("deprecation")
    public void onDismiss() {
        mPopup = null;
        mMenu.close();
        if (mTreeObserver != null) {
            if (!mTreeObserver.isAlive()) {
                mTreeObserver = mAnchorView.getViewTreeObserver();
            }
            mTreeObserver.removeGlobalOnLayoutListener(this);
            mTreeObserver = null;
        }
        if (mAnchorView instanceof View_HasStateListenerSupport) {
            ((View_HasStateListenerSupport) mAnchorView)
                    .removeOnAttachStateChangeListener(this);
        }
    }

    @Override
    public void onGlobalLayout() {
        if (isShowing()) {
            final View anchor = mAnchorView;
            if (anchor == null || !anchor.isShown()) {
                dismiss();
            } else if (isShowing()) {
                mPopup.show();
            }
        }
    }

    @Override
    public void onItemClick(AdapterView<?> parent, View view, int position,
            long id) {
        MenuAdapter adapter = mAdapter;
        adapter.mAdapterMenu.performItemAction(adapter.getItem(position), 0);
    }

    @Override
    public boolean onKey(View v, int keyCode, KeyEvent event) {
        if (event.getAction() == KeyEvent.ACTION_UP
                && keyCode == KeyEvent.KEYCODE_MENU) {
            dismiss();
            return true;
        }
        return false;
    }

    @Override
    public void onRestoreInstanceState(Parcelable state) {
    }

    @Override
    public Parcelable onSaveInstanceState() {
        return null;
    }

    @Override
    public boolean onSubMenuSelected(SubMenuBuilder subMenu) {
        if (subMenu.hasVisibleItems()) {
            PopupMenuHelper subPopup = new PopupMenuHelper(mContext, subMenu,
                    mAnchorView);
            subPopup.setCallback(mPresenterCallback);
            boolean preserveIconSpacing = false;
            final int count = subMenu.size();
            for (int i = 0; i < count; i++) {
                MenuItem childItem = subMenu.getItem(i);
                if (childItem.isVisible() && childItem.getIcon() != null) {
                    preserveIconSpacing = true;
                    break;
                }
            }
            subPopup.setForceShowIcon(preserveIconSpacing);
            if (subPopup.tryShow()) {
                if (mPresenterCallback != null) {
                    mPresenterCallback.onOpenSubMenu(subMenu);
                }
                return true;
            }
        }
        return false;
    }

    @Override
    public void onViewAttachedToWindow(View v) {
    }

    @SuppressWarnings("deprecation")
    @Override
    public void onViewDetachedFromWindow(View v) {
        if (mTreeObserver != null) {
            if (!mTreeObserver.isAlive()) {
                mTreeObserver = v.getViewTreeObserver();
            }
            mTreeObserver.removeGlobalOnLayoutListener(this);
        }
        if (v instanceof View_HasStateListenerSupport) {
            ((View_HasStateListenerSupport) v)
                    .removeOnAttachStateChangeListener(this);
        }
    }

    @Override
    public void setCallback(Callback cb) {
        mPresenterCallback = cb;
    }

    public void setForceShowIcon(boolean forceShow) {
        mForceShowIcon = forceShow;
    }

    public void show() {
        if (!tryShow()) {
            throw new IllegalStateException(
                    "PopupMenuHelper cannot be used without an anchor");
        }
    }

    public boolean tryShow() {
        mPopup = new ListPopupWindow(mContext);
        mPopup.setOnDismissListener(this);
        mPopup.setOnItemClickListener(this);
        mAdapter = new MenuAdapter(mMenu);
        mPopup.setAdapter(mAdapter);
        mPopup.setModal(true);
        View anchor = mAnchorView;
        if (anchor != null) {
            final boolean addGlobalListener = mTreeObserver == null;
            mTreeObserver = anchor.getViewTreeObserver();
            if (addGlobalListener) {
                mTreeObserver.addOnGlobalLayoutListener(this);
                if (anchor instanceof View_HasStateListenerSupport) {
                    ((View_HasStateListenerSupport) anchor)
                            .addOnAttachStateChangeListener(this);
                }
            }
            mPopup.setAnchorView(anchor);
        } else {
            return false;
        }
        mPopup.setContentWidth(Math.min(measureContentWidth(mAdapter),
                mPopupMaxWidth));
        mPopup.setInputMethodMode(PopupWindow.INPUT_METHOD_NOT_NEEDED);
        mPopup.show();
        mPopup.getListView().setOnKeyListener(this);
        return true;
    }

    @Override
    public void updateMenuView(boolean cleared) {
        if (mAdapter != null) {
            mAdapter.notifyDataSetChanged();
        }
    }
}
