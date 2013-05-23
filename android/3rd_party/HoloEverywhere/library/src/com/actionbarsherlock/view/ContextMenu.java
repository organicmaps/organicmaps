
package com.actionbarsherlock.view;

import android.graphics.drawable.Drawable;
import android.view.View;

public interface ContextMenu extends Menu {
    public void clearHeader();

    public ContextMenu setHeaderIcon(Drawable icon);

    public ContextMenu setHeaderIcon(int iconRes);

    public ContextMenu setHeaderTitle(CharSequence title);

    public ContextMenu setHeaderTitle(int titleRes);

    public ContextMenu setHeaderView(View view);
}
