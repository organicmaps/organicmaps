
package org.holoeverywhere.widget;

import org.holoeverywhere.R;
import org.holoeverywhere.internal._View;

import android.content.Context;
import android.content.res.TypedArray;
import android.util.AttributeSet;
import android.util.TypedValue;

public class Divider extends _View {
    public Divider(Context context) {
        super(context);
        init(context, null, 0);
    }

    public Divider(Context context, AttributeSet attrs) {
        super(context, attrs);
        init(context, attrs, 0);
    }

    public Divider(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        init(context, attrs, defStyle);
    }

    protected void init(Context context, AttributeSet attrs, int defStyle) {
        TypedArray a = context.obtainStyledAttributes(attrs,
                R.styleable.Divider, defStyle, 0);
        boolean vertical = a.getInt(R.styleable.Divider_android_orientation,
                android.widget.LinearLayout.VERTICAL) == android.widget.LinearLayout.VERTICAL;
        a.recycle();
        TypedValue value = new TypedValue();
        context.getTheme().resolveAttribute(
                vertical ? R.attr.dividerVertical : R.attr.dividerHorizontal,
                value, true);
        if (value.resourceId > 0) {
            setBackgroundResource(value.resourceId);
        }
    }
}
