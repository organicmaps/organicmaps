
package org.holoeverywhere.widget;

import org.holoeverywhere.R;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.res.TypedArray;
import android.os.Build.VERSION;
import android.util.AttributeSet;

public class RadioButton extends android.widget.RadioButton {
    private boolean allCaps = false;
    private CharSequence originalText;
    private BufferType originalType;

    public RadioButton(Context context) {
        this(context, null);
    }

    public RadioButton(Context context, AttributeSet attrs) {
        this(context, attrs, android.R.attr.radioButtonStyle);
    }

    public RadioButton(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        TypedArray a = getContext().obtainStyledAttributes(attrs,
                R.styleable.TextView, defStyle, 0);
        if (a.hasValue(R.styleable.TextView_android_textAllCaps)) {
            allCaps = a.getBoolean(R.styleable.TextView_android_textAllCaps,
                    false);
        } else {
            allCaps = a.getBoolean(R.styleable.TextView_textAllCaps, false);
        }
        CharSequence text = null;
        if (a.hasValue(R.styleable.TextView_android_text)) {
            text = a.getText(R.styleable.TextView_android_text);
        }
        a.recycle();
        if (text != null) {
            setText(text);
        }
    }

    @Override
    @SuppressLint("NewApi")
    public void dispatchDisplayHint(int hint) {
        onDisplayHint(hint);
    }

    public boolean isAllCaps() {
        return allCaps;
    }

    @Override
    @SuppressLint("NewApi")
    protected void onDisplayHint(int hint) {
        if (VERSION.SDK_INT >= 8) {
            super.onDisplayHint(hint);
        }
    }

    @Override
    public void setAllCaps(boolean allCaps) {
        this.allCaps = allCaps;
        updateTextState();
    }

    @Override
    public void setText(CharSequence text, BufferType type) {
        originalText = text;
        originalType = type;
        updateTextState();
    }

    private void updateTextState() {
        if (originalText == null) {
            super.setText(null, originalType);
            return;
        }
        super.setText(allCaps ? originalText.toString().toUpperCase()
                : originalText, originalType);
    }
}
