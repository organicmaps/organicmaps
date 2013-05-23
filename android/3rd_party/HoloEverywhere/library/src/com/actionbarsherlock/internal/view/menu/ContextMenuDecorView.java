
package com.actionbarsherlock.internal.view.menu;

import org.holoeverywhere.HoloEverywhere;
import org.holoeverywhere.widget.FrameLayout;

import android.content.Context;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;

import com.actionbarsherlock.view.ContextMenu;
import com.actionbarsherlock.view.MenuItem;

public class ContextMenuDecorView extends FrameLayout implements
        MenuPresenter.Callback, MenuBuilder.Callback {
    public interface ContextMenuListenersProvider {
        public ContextMenuListener getContextMenuListener(View view);
    }

    private ContextMenuBuilder mContextMenu;
    private ContextMenuListener mListener;
    private MenuDialogHelper mMenuDialogHelper;
    private ContextMenuListenersProvider mProvider;

    public ContextMenuDecorView(Context context) {
        super(context);
        setLayoutParams(new FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT));
    }

    @Override
    public void onCloseMenu(MenuBuilder menu, boolean allMenusAreClosing) {
        if (mListener == null) {
            return;
        }
        mListener.onContextMenuClosed((ContextMenu) menu);
    }

    @Override
    public boolean onInterceptTouchEvent(MotionEvent ev) {
        return false;
    }

    @Override
    public boolean onMenuItemSelected(MenuBuilder menu, MenuItem item) {
        if (mListener == null) {
            return false;
        }
        if (menu instanceof ContextMenuBuilder
                && item instanceof MenuItemImpl) {
            ((MenuItemImpl) item).setMenuInfo(((ContextMenuBuilder) menu)
                    .getContextMenuInfo());
        }
        return mListener.onContextItemSelected(item);
    }

    @Override
    public void onMenuModeChange(MenuBuilder menu) {

    }

    @Override
    public boolean onOpenSubMenu(MenuBuilder subMenu) {
        return false;
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        return false;
    }

    public void setProvider(ContextMenuListenersProvider provider) {
        mProvider = provider;
    }

    @Override
    public boolean showContextMenuForChild(View originalView) {
        if (HoloEverywhere.WRAP_TO_NATIVE_CONTEXT_MENU) {
            return super.showContextMenuForChild(originalView);
        }
        mListener = mProvider.getContextMenuListener(originalView);
        if (mListener == null) {
            return false;
        }
        if (mContextMenu == null) {
            mContextMenu = new ContextMenuBuilder(getContext(), mListener);
            mContextMenu.setCallback(this);
        } else {
            mContextMenu.clearAll();
            mContextMenu.setContextMenuListener(mListener);
        }
        mMenuDialogHelper = mContextMenu.show(originalView, originalView.getWindowToken());
        if (mMenuDialogHelper != null) {
            mMenuDialogHelper.setPresenterCallback(this);
            return true;
        } else {
            return false;
        }
    }
}
