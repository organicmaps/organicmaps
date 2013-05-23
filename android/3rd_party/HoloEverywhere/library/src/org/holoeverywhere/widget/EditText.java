
package org.holoeverywhere.widget;

import org.holoeverywhere.R;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.res.TypedArray;
import android.os.Build.VERSION;
import android.util.AttributeSet;

import com.actionbarsherlock.internal.view.menu.ContextMenuCallbackGetter;

public class EditText extends android.widget.EditText implements ContextMenuCallbackGetter {
    private boolean allCaps = false;
    private OnCreateContextMenuListener mOnCreateContextMenuListener;
    private CharSequence originalText;

    private BufferType originalType;

    public EditText(Context context) {
        this(context, null);
    }

    public EditText(Context context, AttributeSet attrs) {
        this(context, attrs, android.R.attr.editTextStyle);
    }

    public EditText(Context context, AttributeSet attrs, int defStyle) {
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

    @Override
    public OnCreateContextMenuListener getOnCreateContextMenuListener() {
        return mOnCreateContextMenuListener;
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
    public void setOnCreateContextMenuListener(OnCreateContextMenuListener l) {
        super.setOnCreateContextMenuListener(mOnCreateContextMenuListener = l);
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
