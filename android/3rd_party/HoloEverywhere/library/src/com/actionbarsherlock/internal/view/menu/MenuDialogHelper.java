
package com.actionbarsherlock.internal.view.menu;

import org.holoeverywhere.R;
import org.holoeverywhere.app.AlertDialog;

import android.content.DialogInterface;
import android.os.IBinder;
import android.view.KeyEvent;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;

public class MenuDialogHelper implements DialogInterface.OnKeyListener,
        DialogInterface.OnClickListener, DialogInterface.OnDismissListener,
        MenuPresenter.Callback {
    private AlertDialog mDialog;
    private MenuBuilder mMenu;
    ListMenuPresenter mPresenter;
    private MenuPresenter.Callback mPresenterCallback;

    public MenuDialogHelper(MenuBuilder menu) {
        mMenu = menu;
    }

    public void dismiss() {
        if (mDialog != null) {
            mDialog.dismiss();
        }
    }

    @Override
    public void onClick(DialogInterface dialog, int which) {
        mMenu.performItemAction(
                (MenuItemImpl) mPresenter.getAdapter().getItem(which), 0);
    }

    @Override
    public void onCloseMenu(MenuBuilder menu, boolean allMenusAreClosing) {
        if (allMenusAreClosing || menu == mMenu) {
            dismiss();
        }
        if (mPresenterCallback != null) {
            mPresenterCallback.onCloseMenu(menu, allMenusAreClosing);
        }
    }

    @Override
    public void onDismiss(DialogInterface dialog) {
        mPresenter.onCloseMenu(mMenu, true);
    }

    @Override
    public boolean onKey(DialogInterface dialog, int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_MENU
                || keyCode == KeyEvent.KEYCODE_BACK) {
            if (event.getAction() == KeyEvent.ACTION_DOWN
                    && event.getRepeatCount() == 0) {
                Window win = mDialog.getWindow();
                if (win != null) {
                    View decor = win.getDecorView();
                    if (decor != null) {
                        KeyEvent.DispatcherState ds = decor
                                .getKeyDispatcherState();
                        if (ds != null) {
                            ds.startTracking(event, this);
                            return true;
                        }
                    }
                }
            } else if (event.getAction() == KeyEvent.ACTION_UP
                    && !event.isCanceled()) {
                Window win = mDialog.getWindow();
                if (win != null) {
                    View decor = win.getDecorView();
                    if (decor != null) {
                        KeyEvent.DispatcherState ds = decor
                                .getKeyDispatcherState();
                        if (ds != null && ds.isTracking(event)) {
                            mMenu.close(true);
                            dialog.dismiss();
                            return true;
                        }
                    }
                }
            }
        }
        return mMenu.performShortcut(keyCode, event, 0);

    }

    @Override
    public boolean onOpenSubMenu(MenuBuilder subMenu) {
        if (mPresenterCallback != null) {
            return mPresenterCallback.onOpenSubMenu(subMenu);
        }
        return false;
    }

    public void setPresenterCallback(MenuPresenter.Callback cb) {
        mPresenterCallback = cb;
    }

    public void show(IBinder windowToken) {
        final MenuBuilder menu = mMenu;
        final AlertDialog.Builder builder = new AlertDialog.Builder(
                menu.getContext());
        mPresenter = new ListMenuPresenter(builder.getContext(),
                R.layout.list_menu_item_layout);
        mPresenter.setCallback(this);
        mMenu.addMenuPresenter(mPresenter);
        builder.setAdapter(mPresenter.getAdapter(), this);
        final View headerView = menu.getHeaderView();
        if (headerView != null) {
            builder.setCustomTitle(headerView);
        } else {
            builder.setIcon(menu.getHeaderIcon()).setTitle(
                    menu.getHeaderTitle());
        }
        builder.setOnKeyListener(this);
        builder.setOnDismissListener(this);
        mDialog = builder.create();
        WindowManager.LayoutParams lp = mDialog.getWindow().getAttributes();
        lp.type = WindowManager.LayoutParams.TYPE_APPLICATION_ATTACHED_DIALOG;
        if (windowToken != null) {
            lp.token = windowToken;
        }
        lp.flags |= WindowManager.LayoutParams.FLAG_ALT_FOCUSABLE_IM;
        mDialog.show();
    }
}
