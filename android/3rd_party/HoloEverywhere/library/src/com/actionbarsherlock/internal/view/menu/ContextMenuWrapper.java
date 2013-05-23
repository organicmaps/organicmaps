
package com.actionbarsherlock.internal.view.menu;

import android.graphics.drawable.Drawable;
import android.view.View;

import com.actionbarsherlock.view.ContextMenu;

public class ContextMenuWrapper extends MenuWrapper implements ContextMenu {
    private android.view.ContextMenu nativeMenu;

    public ContextMenuWrapper(android.view.ContextMenu nativeMenu) {
        super(nativeMenu);
        this.nativeMenu = nativeMenu;
    }

    @Override
    public void clearHeader() {
        nativeMenu.clearHeader();
    }

    @Override
    public ContextMenu setHeaderIcon(Drawable icon) {
        nativeMenu.setHeaderIcon(icon);
        return this;
    }

    @Override
    public ContextMenu setHeaderIcon(int iconRes) {
        nativeMenu.setHeaderIcon(iconRes);
        return this;
    }

    @Override
    public ContextMenu setHeaderTitle(CharSequence title) {
        nativeMenu.setHeaderTitle(title);
        return this;
    }

    @Override
    public ContextMenu setHeaderTitle(int titleRes) {
        nativeMenu.setHeaderTitle(titleRes);
        return this;
    }

    @Override
    public ContextMenu setHeaderView(View view) {
        nativeMenu.setHeaderView(view);
        return this;
    }

    @Override
    public android.view.ContextMenu unwrap() {
        return nativeMenu;
    }
}
