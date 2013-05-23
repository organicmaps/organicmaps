
package com.actionbarsherlock.internal.view.menu;

import android.view.ContextMenu.ContextMenuInfo;
import android.view.View;

import com.actionbarsherlock.view.ContextMenu;
import com.actionbarsherlock.view.MenuItem;

public interface ContextMenuListener {
    public boolean onContextItemSelected(MenuItem item);

    public void onContextMenuClosed(ContextMenu menu);

    public void onCreateContextMenu(ContextMenu menu, View view, ContextMenuInfo menuInfo);
}
