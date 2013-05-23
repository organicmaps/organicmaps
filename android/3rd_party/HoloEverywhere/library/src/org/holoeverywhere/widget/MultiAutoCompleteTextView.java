
package org.holoeverywhere.widget;

import android.annotation.SuppressLint;
import android.content.Context;
import android.text.Editable;
import android.text.SpannableString;
import android.text.Spanned;
import android.text.TextUtils;
import android.text.method.QwertyKeyListener;
import android.util.AttributeSet;
import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityNodeInfo;
import android.widget.Filter;

public class MultiAutoCompleteTextView extends AutoCompleteTextView {
    public static class CommaTokenizer implements Tokenizer {
        @Override
        public int findTokenEnd(CharSequence text, int cursor) {
            int i = cursor;
            int len = text.length();
            while (i < len) {
                if (text.charAt(i) == ',') {
                    return i;
                } else {
                    i++;
                }
            }
            return len;
        }

        @Override
        public int findTokenStart(CharSequence text, int cursor) {
            int i = cursor;
            while (i > 0 && text.charAt(i - 1) != ',') {
                i--;
            }
            while (i < cursor && text.charAt(i) == ' ') {
                i++;
            }
            return i;
        }

        @Override
        public CharSequence terminateToken(CharSequence text) {
            int i = text.length();
            while (i > 0 && text.charAt(i - 1) == ' ') {
                i--;
            }
            if (i > 0 && text.charAt(i - 1) == ',') {
                return text;
            } else {
                if (text instanceof Spanned) {
                    SpannableString sp = new SpannableString(text + ", ");
                    TextUtils.copySpansFrom((Spanned) text, 0, text.length(),
                            Object.class, sp, 0);
                    return sp;
                } else {
                    return text + ", ";
                }
            }
        }
    }

    public static interface Tokenizer {
        public int findTokenEnd(CharSequence text, int cursor);

        public int findTokenStart(CharSequence text, int cursor);

        public CharSequence terminateToken(CharSequence text);
    }

    private Tokenizer mTokenizer;

    public MultiAutoCompleteTextView(Context context) {
        super(context);
    }

    public MultiAutoCompleteTextView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public MultiAutoCompleteTextView(Context context, AttributeSet attrs,
            int defStyle) {
        super(context, attrs, defStyle);
    }

    @Override
    public boolean enoughToFilter() {
        Editable text = getText();
        int end = getSelectionEnd();
        if (end < 0 || mTokenizer == null) {
            return false;
        }
        int start = mTokenizer.findTokenStart(text, end);
        if (end - start >= getThreshold()) {
            return true;
        } else {
            return false;
        }
    }

    @Override
    public void onInitializeAccessibilityEvent(AccessibilityEvent event) {
        super.onInitializeAccessibilityEvent(event);
        event.setClassName(MultiAutoCompleteTextView.class.getName());
    }

    @Override
    @SuppressLint("NewApi")
    public void onInitializeAccessibilityNodeInfo(AccessibilityNodeInfo info) {
        super.onInitializeAccessibilityNodeInfo(info);
        info.setClassName(MultiAutoCompleteTextView.class.getName());
    }

    @Override
    protected void onLoadEntries(CharSequence[] entries) {
        super.onLoadEntries(entries);
        if (mTokenizer == null) {
            setTokenizer(new CommaTokenizer());
        }
    }

    @Override
    protected void performFiltering(CharSequence text, int keyCode) {
        if (enoughToFilter()) {
            int end = getSelectionEnd();
            int start = mTokenizer.findTokenStart(text, end);
            performFiltering(text, start, end, keyCode);
        } else {
            dismissDropDown();
            Filter f = getFilter();
            if (f != null) {
                f.filter(null);
            }
        }
    }

    protected void performFiltering(CharSequence text, int start, int end,
            int keyCode) {
        getFilter().filter(text.subSequence(start, end), this);
    }

    @Override
    public void performValidation() {
        Validator v = getValidator();
        if (v == null || mTokenizer == null) {
            return;
        }
        Editable e = getText();
        int i = getText().length();
        while (i > 0) {
            int start = mTokenizer.findTokenStart(e, i);
            int end = mTokenizer.findTokenEnd(e, start);
            CharSequence sub = e.subSequence(start, end);
            if (TextUtils.isEmpty(sub)) {
                e.replace(start, i, "");
            } else if (!v.isValid(sub)) {
                e.replace(start, i, mTokenizer.terminateToken(v.fixText(sub)));
            }
            i = start;
        }
    }

    @Override
    protected void replaceText(CharSequence text) {
        clearComposingText();
        int end = getSelectionEnd();
        int start = mTokenizer.findTokenStart(getText(), end);
        Editable editable = getText();
        String original = TextUtils.substring(editable, start, end);
        QwertyKeyListener.markAsReplaced(editable, start, end, original);
        editable.replace(start, end, mTokenizer.terminateToken(text));
    }

    public void setTokenizer(Tokenizer t) {
        mTokenizer = t;
    }
}
