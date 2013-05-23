
package org.holoeverywhere.widget;

import org.holoeverywhere.ArrayAdapter;
import org.holoeverywhere.LayoutInflater;
import org.holoeverywhere.R;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.res.TypedArray;
import android.database.DataSetObserver;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.os.Build.VERSION;
import android.text.Editable;
import android.text.InputType;
import android.text.Selection;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.util.AttributeSet;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.view.inputmethod.CompletionInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.AdapterView;
import android.widget.Filter;
import android.widget.Filterable;
import android.widget.ListAdapter;

public class AutoCompleteTextView extends EditText implements
        Filter.FilterListener {
    private class DropDownItemClickListener implements
            AdapterView.OnItemClickListener {
        @Override
        public void onItemClick(AdapterView<?> parent, View v, int position,
                long id) {
            performCompletion(v, position, id);
        }
    }

    private class MyWatcher implements TextWatcher {
        @Override
        public void afterTextChanged(Editable s) {
            doAfterTextChanged();
        }

        @Override
        public void beforeTextChanged(CharSequence s, int start, int count,
                int after) {
            doBeforeTextChanged();
        }

        @Override
        public void onTextChanged(CharSequence s, int start, int before,
                int count) {
        }
    }

    private class PassThroughClickListener implements OnClickListener {
        private View.OnClickListener mWrapped;

        @Override
        public void onClick(View v) {
            onClickImpl();

            if (mWrapped != null) {
                mWrapped.onClick(v);
            }
        }
    }

    private class PopupDataSetObserver extends DataSetObserver {
        @Override
        public void onChanged() {
            if (mAdapter != null) {
                post(new Runnable() {
                    @Override
                    public void run() {
                        final ListAdapter adapter = mAdapter;
                        if (adapter != null) {
                            updateDropDownForFilter(adapter.getCount());
                        }
                    }
                });
            }
        }
    }

    public interface Validator {
        CharSequence fixText(CharSequence invalidText);

        boolean isValid(CharSequence text);
    }

    static final int EXPAND_MAX = 3;
    static final String TAG = "AutoCompleteTextView";
    private ListAdapter mAdapter;
    private boolean mBlockCompletion;
    private int mDropDownAnchorId;
    private boolean mDropDownDismissedOnCompletion = true;
    private Filter mFilter;
    private int mHintResource;
    private CharSequence mHintText;
    private TextView mHintView;
    private AdapterView.OnItemClickListener mItemClickListener;
    private AdapterView.OnItemSelectedListener mItemSelectedListener;
    private int mLastKeyCode = KeyEvent.KEYCODE_UNKNOWN;
    private PopupDataSetObserver mObserver;
    private boolean mOpenBefore;

    private PassThroughClickListener mPassThroughClickListener;

    private ListPopupWindow mPopup;

    private boolean mPopupCanBeUpdated = true;

    private int mThreshold;

    private Validator mValidator = null;

    public AutoCompleteTextView(Context context) {
        this(context, null);
    }

    public AutoCompleteTextView(Context context, AttributeSet attrs) {
        this(context, attrs, R.attr.autoCompleteTextViewStyle);
    }

    public AutoCompleteTextView(Context context, AttributeSet attrs,
            int defStyle) {
        super(context, attrs, defStyle);
        mPopup = new ListPopupWindow(context, attrs,
                R.attr.autoCompleteTextViewStyle);
        mPopup.setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE);
        mPopup.setPromptPosition(ListPopupWindow.POSITION_PROMPT_BELOW);
        TypedArray a = context.obtainStyledAttributes(attrs,
                R.styleable.AutoCompleteTextView, defStyle, 0);
        mThreshold = a
                .getInt(R.styleable.AutoCompleteTextView_android_completionThreshold,
                        2);
        mPopup.setListSelector(a
                .getDrawable(R.styleable.AutoCompleteTextView_android_dropDownSelector));
        mPopup.setVerticalOffset((int) a.getDimension(
                R.styleable.AutoCompleteTextView_dropDownVerticalOffset, 0.0f));
        mPopup.setHorizontalOffset((int) a
                .getDimension(
                        R.styleable.AutoCompleteTextView_dropDownHorizontalOffset,
                        0.0f));
        mDropDownAnchorId = a.getResourceId(
                R.styleable.AutoCompleteTextView_android_dropDownAnchor,
                View.NO_ID);
        mPopup.setWidth(a.getLayoutDimension(
                R.styleable.AutoCompleteTextView_android_dropDownWidth,
                ViewGroup.LayoutParams.WRAP_CONTENT));
        mPopup.setHeight(a.getLayoutDimension(
                R.styleable.AutoCompleteTextView_android_dropDownHeight,
                ViewGroup.LayoutParams.WRAP_CONTENT));
        mHintResource = a.getResourceId(
                R.styleable.AutoCompleteTextView_android_completionHintView,
                R.layout.simple_dropdown_hint);
        mPopup.setOnItemClickListener(new DropDownItemClickListener());
        setCompletionHint(a
                .getText(R.styleable.AutoCompleteTextView_android_completionHint));
        int inputType = getInputType();
        if ((inputType & InputType.TYPE_MASK_CLASS) == InputType.TYPE_CLASS_TEXT) {
            inputType |= InputType.TYPE_TEXT_FLAG_AUTO_COMPLETE;
            setRawInputType(inputType);
        }
        CharSequence[] entries = null;
        if (a.hasValue(R.styleable.AutoCompleteTextView_android_entries)) {
            entries = a
                    .getTextArray(R.styleable.AutoCompleteTextView_android_entries);
        }
        a.recycle();
        setFocusable(true);
        addTextChangedListener(new MyWatcher());
        mPassThroughClickListener = new PassThroughClickListener();
        super.setOnClickListener(mPassThroughClickListener);
        if (entries != null) {
            onLoadEntries(entries);
        }
    }

    private void buildImeCompletions() {
        final ListAdapter adapter = mAdapter;
        if (adapter != null) {
            InputMethodManager imm = (InputMethodManager) getContext()
                    .getSystemService(Context.INPUT_METHOD_SERVICE);
            if (imm != null) {
                final int count = Math.min(adapter.getCount(), 20);
                CompletionInfo[] completions = new CompletionInfo[count];
                int realCount = 0;
                for (int i = 0; i < count; i++) {
                    if (adapter.isEnabled(i)) {
                        Object item = adapter.getItem(i);
                        long id = adapter.getItemId(i);
                        completions[realCount] = new CompletionInfo(id,
                                realCount, convertSelectionToString(item));
                        realCount++;
                    }
                }
                if (realCount != count) {
                    CompletionInfo[] tmp = new CompletionInfo[realCount];
                    System.arraycopy(completions, 0, tmp, 0, realCount);
                    completions = tmp;
                }

                imm.displayCompletions(this, completions);
            }
        }
    }

    public void clearListSelection() {
        mPopup.clearListSelection();
    }

    protected CharSequence convertSelectionToString(Object selectedItem) {
        return mFilter.convertResultToString(selectedItem);
    }

    public void dismissDropDown() {
        InputMethodManager imm = (InputMethodManager) getContext()
                .getSystemService(Context.INPUT_METHOD_SERVICE);
        if (imm != null) {
            imm.displayCompletions(this, null);
        }
        mPopup.dismiss();
        mPopupCanBeUpdated = false;
    }

    void doAfterTextChanged() {
        if (mBlockCompletion) {
            return;
        }
        if (mOpenBefore && !isPopupShowing()) {
            return;
        }
        if (enoughToFilter()) {
            if (mFilter != null) {
                mPopupCanBeUpdated = true;
                performFiltering(getText(), mLastKeyCode);
            }
        } else {
            if (!mPopup.isDropDownAlwaysVisible()) {
                dismissDropDown();
            }
            if (mFilter != null) {
                mFilter.filter(null);
            }
        }
    }

    void doBeforeTextChanged() {
        if (mBlockCompletion) {
            return;
        }
        mOpenBefore = isPopupShowing();
    }

    public boolean enoughToFilter() {
        return getText().length() >= mThreshold;
    }

    public void ensureImeVisible(boolean visible) {
        mPopup.setInputMethodMode(visible ? ListPopupWindow.INPUT_METHOD_NEEDED
                : ListPopupWindow.INPUT_METHOD_NOT_NEEDED);
        if (mPopup.isDropDownAlwaysVisible() || mFilter != null
                && enoughToFilter()) {
            showDropDown();
        }
    }

    public ListAdapter getAdapter() {
        return mAdapter;
    }

    public CharSequence getCompletionHint() {
        return mHintText;
    }

    public int getDropDownAnchor() {
        return mDropDownAnchorId;
    }

    public int getDropDownAnimationStyle() {
        return mPopup.getAnimationStyle();
    }

    public Drawable getDropDownBackground() {
        return mPopup.getBackground();
    }

    public int getDropDownHeight() {
        return mPopup.getHeight();
    }

    public int getDropDownHorizontalOffset() {
        return mPopup.getHorizontalOffset();
    }

    public int getDropDownVerticalOffset() {
        return mPopup.getVerticalOffset();
    }

    public int getDropDownWidth() {
        return mPopup.getWidth();
    }

    protected Filter getFilter() {
        return mFilter;
    }

    public int getListSelection() {
        return mPopup.getSelectedItemPosition();
    }

    public AdapterView.OnItemClickListener getOnItemClickListener() {
        return mItemClickListener;
    }

    public AdapterView.OnItemSelectedListener getOnItemSelectedListener() {
        return mItemSelectedListener;
    }

    public int getThreshold() {
        return mThreshold;
    }

    public Validator getValidator() {
        return mValidator;
    }

    public boolean isDropDownAlwaysVisible() {
        return mPopup.isDropDownAlwaysVisible();
    }

    public boolean isDropDownDismissedOnCompletion() {
        return mDropDownDismissedOnCompletion;
    }

    public boolean isInputMethodNotNeeded() {
        return mPopup.getInputMethodMode() == ListPopupWindow.INPUT_METHOD_NOT_NEEDED;
    }

    public boolean isPerformingCompletion() {
        return mBlockCompletion;
    }

    public boolean isPopupShowing() {
        return mPopup.isShowing();
    }

    private void onClickImpl() {
        if (isPopupShowing()) {
            ensureImeVisible(true);
        }
    }

    @Override
    public void onCommitCompletion(CompletionInfo completion) {
        if (isPopupShowing()) {
            mPopup.performItemClick(completion.getPosition());
        }
    }

    @Override
    protected void onDetachedFromWindow() {
        dismissDropDown();
        super.onDetachedFromWindow();
    }

    @Override
    protected void onDisplayHint(int hint) {
        super.onDisplayHint(hint);
        switch (hint) {
            case INVISIBLE:
                if (!mPopup.isDropDownAlwaysVisible()) {
                    dismissDropDown();
                }
                break;
        }
    }

    @Override
    public void onFilterComplete(int count) {
        updateDropDownForFilter(count);
    }

    @Override
    protected void onFocusChanged(boolean focused, int direction,
            Rect previouslyFocusedRect) {
        super.onFocusChanged(focused, direction, previouslyFocusedRect);
        if (!focused) {
            performValidation();
        }
        if (!focused && !mPopup.isDropDownAlwaysVisible()) {
            dismissDropDown();
        }
    }

    @Override
    @SuppressLint("NewApi")
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (mPopup.onKeyDown(keyCode, event)) {
            return true;
        }
        if (!isPopupShowing()) {
            switch (keyCode) {
                case KeyEvent.KEYCODE_DPAD_DOWN:
                    if (VERSION.SDK_INT < 11 || event.hasNoModifiers()) {
                        performValidation();
                    }
            }
        }
        if (isPopupShowing() && keyCode == KeyEvent.KEYCODE_TAB
                && (VERSION.SDK_INT < 11 || event.hasNoModifiers())) {
            return true;
        }
        mLastKeyCode = keyCode;
        boolean handled = super.onKeyDown(keyCode, event);
        mLastKeyCode = KeyEvent.KEYCODE_UNKNOWN;
        if (handled && isPopupShowing()) {
            clearListSelection();
        }
        return handled;
    }

    @Override
    @SuppressLint("NewApi")
    public boolean onKeyPreIme(int keyCode, KeyEvent event) {
        if (VERSION.SDK_INT < 5) {
            return false;
        }
        if (keyCode == KeyEvent.KEYCODE_BACK && isPopupShowing()
                && !mPopup.isDropDownAlwaysVisible()) {
            if (event.getAction() == KeyEvent.ACTION_DOWN
                    && event.getRepeatCount() == 0) {
                KeyEvent.DispatcherState state = getKeyDispatcherState();
                if (state != null) {
                    state.startTracking(event, this);
                }
                return true;
            } else if (event.getAction() == KeyEvent.ACTION_UP) {
                KeyEvent.DispatcherState state = getKeyDispatcherState();
                if (state != null) {
                    state.handleUpEvent(event);
                }
                if (event.isTracking() && !event.isCanceled()) {
                    dismissDropDown();
                    return true;
                }
            }
        }
        return super.onKeyPreIme(keyCode, event);
    }

    @Override
    @SuppressLint("NewApi")
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        boolean consumed = mPopup.onKeyUp(keyCode, event);
        if (consumed) {
            switch (keyCode) {
                case KeyEvent.KEYCODE_ENTER:
                case KeyEvent.KEYCODE_DPAD_CENTER:
                case KeyEvent.KEYCODE_TAB:
                    if (VERSION.SDK_INT < 11 || event.hasNoModifiers()) {
                        performCompletion();
                    }
                    return true;
            }
        }
        if (isPopupShowing() && keyCode == KeyEvent.KEYCODE_TAB
                && (VERSION.SDK_INT < 11 || event.hasNoModifiers())) {
            performCompletion();
            return true;
        }
        return super.onKeyUp(keyCode, event);
    }

    protected void onLoadEntries(CharSequence[] entries) {
        setAdapter(new ArrayAdapter<CharSequence>(getContext(),
                R.layout.simple_dropdown_item_1line, entries));
    }

    @Override
    public void onWindowFocusChanged(boolean hasWindowFocus) {
        super.onWindowFocusChanged(hasWindowFocus);
        if (!hasWindowFocus && !mPopup.isDropDownAlwaysVisible()) {
            dismissDropDown();
        }
    }

    public void performCompletion() {
        performCompletion(null, -1, -1);
    }

    private void performCompletion(View selectedView, int position, long id) {
        if (isPopupShowing()) {
            Object selectedItem;
            if (position < 0) {
                selectedItem = mPopup.getSelectedItem();
            } else {
                selectedItem = mAdapter.getItem(position);
            }
            if (selectedItem == null) {
                Log.w(AutoCompleteTextView.TAG,
                        "performCompletion: no selected item");
                return;
            }
            mBlockCompletion = true;
            replaceText(convertSelectionToString(selectedItem));
            mBlockCompletion = false;
            if (mItemClickListener != null) {
                final ListPopupWindow list = mPopup;
                if (selectedView == null || position < 0) {
                    selectedView = list.getSelectedView();
                    position = list.getSelectedItemPosition();
                    id = list.getSelectedItemId();
                }
                mItemClickListener.onItemClick(list.getListView(),
                        selectedView, position, id);
            }
        }
        if (mDropDownDismissedOnCompletion && !mPopup.isDropDownAlwaysVisible()) {
            dismissDropDown();
        }
    }

    protected void performFiltering(CharSequence text, int keyCode) {
        mFilter.filter(text, this);
    }

    public void performValidation() {
        if (mValidator == null) {
            return;
        }
        CharSequence text = getText();
        if (!TextUtils.isEmpty(text) && !mValidator.isValid(text)) {
            setText(mValidator.fixText(text));
        }
    }

    protected void replaceText(CharSequence text) {
        clearComposingText();
        setText(text);
        Editable spannable = getText();
        Selection.setSelection(spannable, spannable.length());
    }

    public <T extends ListAdapter & Filterable> void setAdapter(T adapter) {
        if (mObserver == null) {
            mObserver = new PopupDataSetObserver();
        } else if (mAdapter != null) {
            mAdapter.unregisterDataSetObserver(mObserver);
        }
        mAdapter = adapter;
        if (mAdapter != null) {
            mFilter = ((Filterable) mAdapter).getFilter();
            adapter.registerDataSetObserver(mObserver);
        } else {
            mFilter = null;
        }
        mPopup.setAdapter(mAdapter);
    }

    public void setCompletionHint(CharSequence hint) {
        mHintText = hint;
        if (hint != null) {
            if (mHintView == null) {
                final TextView hintView = (TextView) LayoutInflater.inflate(
                        getContext(), mHintResource).findViewById(
                        android.R.id.text1);
                hintView.setText(mHintText);
                mHintView = hintView;
                mPopup.setPromptView(hintView);
            } else {
                mHintView.setText(hint);
            }
        } else {
            mPopup.setPromptView(null);
            mHintView = null;
        }
    }

    public void setDropDownAlwaysVisible(boolean dropDownAlwaysVisible) {
        mPopup.setDropDownAlwaysVisible(dropDownAlwaysVisible);
    }

    public void setDropDownAnchor(int id) {
        mDropDownAnchorId = id;
        mPopup.setAnchorView(null);
    }

    public void setDropDownAnimationStyle(int animationStyle) {
        mPopup.setAnimationStyle(animationStyle);
    }

    public void setDropDownBackgroundDrawable(Drawable d) {
        mPopup.setBackgroundDrawable(d);
    }

    public void setDropDownBackgroundResource(int id) {
        mPopup.setBackgroundDrawable(getResources().getDrawable(id));
    }

    public void setDropDownDismissedOnCompletion(
            boolean dropDownDismissedOnCompletion) {
        mDropDownDismissedOnCompletion = dropDownDismissedOnCompletion;
    }

    public void setDropDownHeight(int height) {
        mPopup.setHeight(height);
    }

    public void setDropDownHorizontalOffset(int offset) {
        mPopup.setHorizontalOffset(offset);
    }

    public void setDropDownVerticalOffset(int offset) {
        mPopup.setVerticalOffset(offset);
    }

    public void setDropDownWidth(int width) {
        mPopup.setWidth(width);
    }

    public void setForceIgnoreOutsideTouch(boolean forceIgnoreOutsideTouch) {
        mPopup.setForceIgnoreOutsideTouch(forceIgnoreOutsideTouch);
    }

    @Override
    protected boolean setFrame(final int l, int t, final int r, int b) {
        boolean result = super.setFrame(l, t, r, b);
        if (isPopupShowing()) {
            showDropDown();
        }
        return result;
    }

    public void setListSelection(int position) {
        mPopup.setSelection(position);
    }

    @Override
    public void setOnClickListener(OnClickListener listener) {
        mPassThroughClickListener.mWrapped = listener;
    }

    public void setOnItemClickListener(AdapterView.OnItemClickListener l) {
        mItemClickListener = l;
    }

    public void setOnItemSelectedListener(AdapterView.OnItemSelectedListener l) {
        mItemSelectedListener = l;
    }

    public void setText(CharSequence text, boolean filter) {
        if (filter) {
            setText(text);
        } else {
            mBlockCompletion = true;
            setText(text);
            mBlockCompletion = false;
        }
    }

    public void setThreshold(int threshold) {
        if (threshold <= 0) {
            threshold = 1;
        }

        mThreshold = threshold;
    }

    public void setValidator(Validator validator) {
        mValidator = validator;
    }

    @SuppressLint("NewApi")
    public void showDropDown() {
        buildImeCompletions();
        if (mPopup.getAnchorView() == null) {
            if (mDropDownAnchorId != View.NO_ID) {
                mPopup.setAnchorView(getRootView().findViewById(
                        mDropDownAnchorId));
            } else {
                mPopup.setAnchorView(this);
            }
        }
        if (!isPopupShowing()) {
            mPopup.setInputMethodMode(ListPopupWindow.INPUT_METHOD_NEEDED);
            mPopup.setListItemExpandMax(AutoCompleteTextView.EXPAND_MAX);
        }
        mPopup.show();
        if (VERSION.SDK_INT >= 9) {
            mPopup.getListView().setOverScrollMode(View.OVER_SCROLL_ALWAYS);
        }
    }

    public void showDropDownAfterLayout() {
        mPopup.postShow();
    }

    private void updateDropDownForFilter(int count) {
        if (getWindowVisibility() == View.GONE) {
            return;
        }
        final boolean dropDownAlwaysVisible = mPopup.isDropDownAlwaysVisible();
        final boolean enoughToFilter = enoughToFilter();
        if ((count > 0 || dropDownAlwaysVisible) && enoughToFilter) {
            if (hasFocus() && hasWindowFocus() && mPopupCanBeUpdated) {
                showDropDown();
            }
        } else if (!dropDownAlwaysVisible && isPopupShowing()) {
            dismissDropDown();
            mPopupCanBeUpdated = true;
        }
    }
}
