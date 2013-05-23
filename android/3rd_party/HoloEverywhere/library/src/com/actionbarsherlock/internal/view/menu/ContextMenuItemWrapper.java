
package com.actionbarsherlock.internal.view.menu;

import android.view.MenuItem;

public class ContextMenuItemWrapper extends MenuItemWrapper {
    private final MenuItem nativeItem;

    public ContextMenuItemWrapper(MenuItem nativeItem) {
        super(nativeItem);
        this.nativeItem = nativeItem;
    }

    public MenuItem unwrap() {
        return nativeItem;
    }
}
