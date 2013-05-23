
package org.holoeverywhere.internal;

import org.holoeverywhere.R;
import org.holoeverywhere.widget.TextView;

import android.content.Context;
import android.content.res.TypedArray;
import android.text.Layout;
import android.util.AttributeSet;
import android.util.TypedValue;

public class DialogTitle extends TextView {
    public DialogTitle(Context context) {
        super(context);
    }

    public DialogTitle(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public DialogTitle(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
        final Layout layout = getLayout();
        if (layout != null) {
            final int lineCount = layout.getLineCount();
            if (lineCount > 0) {
                final int ellipsisCount = layout
                        .getEllipsisCount(lineCount - 1);
                if (ellipsisCount > 0) {
                    setSingleLine(false);
                    setMaxLines(2);
                    final TypedArray a = getContext().obtainStyledAttributes(
                            null, R.styleable.TextAppearance,
                            android.R.attr.textAppearanceMedium,
                            R.style.Holo_TextAppearance_Medium);
                    final int textSize = a.getDimensionPixelSize(
                            R.styleable.TextAppearance_android_textSize, 0);
                    if (textSize != 0) {
                        setTextSize(TypedValue.COMPLEX_UNIT_PX, textSize);
                    }
                    a.recycle();
                    super.onMeasure(widthMeasureSpec, heightMeasureSpec);
                }
            }
        }
    }
}
