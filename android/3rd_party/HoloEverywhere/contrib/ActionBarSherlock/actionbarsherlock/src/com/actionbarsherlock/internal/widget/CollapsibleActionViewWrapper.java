package com.actionbarsherlock.internal.widget;

import android.view.View;
import android.widget.FrameLayout;
import com.actionbarsherlock.view.CollapsibleActionView;

/**
 * Wraps an ABS collapsible action view in a native container that delegates the calls.
 */
public class CollapsibleActionViewWrapper extends FrameLayout implements android.view.CollapsibleActionView {
    private final CollapsibleActionView child;

    public CollapsibleActionViewWrapper(View child) {
        super(child.getContext());
        this.child = (CollapsibleActionView) child;
        addView(child);
    }

    @Override public void onActionViewExpanded() {
        child.onActionViewExpanded();
    }

    @Override public void onActionViewCollapsed() {
        child.onActionViewCollapsed();
    }

    public View unwrap() {
        return getChildAt(0);
    }
}
