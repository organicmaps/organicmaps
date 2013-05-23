
package com.actionbarsherlock.internal.view.menu;

import java.lang.reflect.Method;

import android.annotation.SuppressLint;
import android.content.Context;
import android.graphics.drawable.Drawable;
import android.os.Build.VERSION;
import android.os.IBinder;
import android.util.EventLog;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.View;

import com.actionbarsherlock.view.ContextMenu;

public class ContextMenuBuilder extends MenuBuilder implements ContextMenu {
    public static interface ContextMenuInfoGetter {
        public ContextMenuInfo getContextMenuInfo();
    }

    private ContextMenuInfo mContextMenuInfo;
    private ContextMenuListener mListener;

    public ContextMenuBuilder(Context context, ContextMenuListener listener) {
        super(context);
        setContextMenuListener(listener);
    }

    public ContextMenuInfo getContextMenuInfo() {
        return mContextMenuInfo;
    }

    protected ContextMenuInfo getContextMenuInfo(View view) {
        if (view instanceof ContextMenuInfoGetter) {
            return ((ContextMenuInfoGetter) view).getContextMenuInfo();
        }
        ContextMenuInfo menuInfo = null;
        try {
            Method method = View.class.getDeclaredMethod("getContextMenuInfo");
            method.setAccessible(true);
            menuInfo = (ContextMenuInfo) method.invoke(view);
        } catch (Exception e) {
        }
        return menuInfo;
    }

    public ContextMenuListener getContextMenuListener() {
        return mListener;
    }

    public void setContextMenuListener(ContextMenuListener listener) {
        mListener = listener;
    }

    @Override
    public ContextMenu setHeaderIcon(Drawable icon) {
        return (ContextMenu) super.setHeaderIconInt(icon);
    }

    @Override
    public ContextMenu setHeaderIcon(int iconRes) {
        return (ContextMenu) super.setHeaderIconInt(iconRes);
    }

    @Override
    public ContextMenu setHeaderTitle(CharSequence title) {
        return (ContextMenu) super.setHeaderTitleInt(title);
    }

    @Override
    public ContextMenu setHeaderTitle(int titleRes) {
        return (ContextMenu) super.setHeaderTitleInt(titleRes);
    }

    @Override
    public ContextMenu setHeaderView(View view) {
        return (ContextMenu) super.setHeaderViewInt(view);
    }

    @SuppressLint("NewApi")
    public MenuDialogHelper show(View originalView, IBinder token) {
        if (mListener == null) {
            throw new IllegalStateException(
                    "Cannot show context menu without reference on ContextMenuListener");
        }
        mContextMenuInfo = getContextMenuInfo(originalView);
        mListener.onCreateContextMenu(this, originalView, mContextMenuInfo);
        if (getVisibleItems().size() > 0) {
            if (VERSION.SDK_INT >= 8) {
                EventLog.writeEvent(50001, 1);
            }
            MenuDialogHelper helper = new MenuDialogHelper(this);
            helper.show(token);
            return helper;
        }
        return null;
    }
}
