
package org.holoeverywhere.internal;

import org.holoeverywhere.IHoloActivity;

import android.content.Context;
import android.util.AttributeSet;
import android.view.View;
import android.view.ViewGroup;
import android.view.accessibility.AccessibilityManager;

import com.actionbarsherlock.view.ActionMode;

public abstract class _ViewGroup extends ViewGroup {
    public static final int ACCESSIBILITY_FOCUS_BACKWARD = View.FOCUS_BACKWARD | 0x00000002;
    public static final int ACCESSIBILITY_FOCUS_FORWARD = View.FOCUS_FORWARD | 0x00000002;
    public static final int FLAG_DISALLOW_INTERCEPT = 0x80000;
    public static final int FOCUS_ACCESSIBILITY = 0x00001000;
    public static final int FOCUSABLES_ACCESSIBILITY = 0x00000002;

    public static boolean isAccessibilityManagerEnabled(Context context) {
        boolean enabled = false;
        try {
            enabled = ((AccessibilityManager) context
                    .getSystemService(Context.ACCESSIBILITY_SERVICE))
                    .isEnabled();
        } catch (Exception e) {
        }
        return enabled;
    }

    public _ViewGroup(Context context) {
        this(context, null);
    }

    public _ViewGroup(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public _ViewGroup(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
    }

    public boolean isAccessibilityManagerEnabled() {
        return _ViewGroup.isAccessibilityManagerEnabled(getContext());
    }

    public ActionMode startActionMode(ActionMode.Callback actionModeCallback) {
        return ((IHoloActivity) getContext())
                .startActionMode(actionModeCallback);
    }
}
