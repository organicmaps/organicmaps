
package org.holoeverywhere.widget;

import org.holoeverywhere.R;
import org.holoeverywhere.app.AlertDialog;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.content.res.TypedArray;
import android.database.DataSetObserver;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.util.Log;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityNodeInfo;
import android.widget.AbsListView;
import android.widget.ListAdapter;
import android.widget.SpinnerAdapter;

public class Spinner extends AbsSpinner implements OnClickListener {
    private class DialogPopup implements SpinnerPopup,
            DialogInterface.OnClickListener {
        private ListAdapter mListAdapter;
        private AlertDialog mPopup;
        private CharSequence mPrompt;

        @Override
        public void dismiss() {
            mPopup.dismiss();
            mPopup = null;
        }

        @Override
        public Drawable getBackground() {
            return null;
        }

        @Override
        public CharSequence getHintText() {
            return mPrompt;
        }

        @Override
        public int getHorizontalOffset() {
            return 0;
        }

        @Override
        public int getVerticalOffset() {
            return 0;
        }

        @Override
        public boolean isShowing() {
            return mPopup != null ? mPopup.isShowing() : false;
        }

        @Override
        public void onClick(DialogInterface dialog, final int which) {
            setSelection(which);
            if (mOnItemClickListener != null) {
                performItemClick(null, which, mListAdapter.getItemId(which));
            }
            dismiss();
        }

        @Override
        public void setAdapter(ListAdapter adapter) {
            mListAdapter = adapter;
        }

        @Override
        public void setBackgroundDrawable(Drawable bg) {
            Log.e(Spinner.TAG,
                    "Cannot set popup background for MODE_DIALOG, ignoring");
        }

        @Override
        public void setHorizontalOffset(int px) {
            Log.e(Spinner.TAG,
                    "Cannot set horizontal offset for MODE_DIALOG, ignoring");
        }

        @Override
        public void setPromptText(CharSequence hintText) {
            mPrompt = hintText;
        }

        @Override
        public void setVerticalOffset(int px) {
            Log.e(Spinner.TAG,
                    "Cannot set vertical offset for MODE_DIALOG, ignoring");
        }

        @Override
        public void show() {
            AlertDialog.Builder builder = new AlertDialog.Builder(getContext());
            if (mPrompt != null) {
                builder.setTitle(mPrompt);
            }
            mPopup = builder.setSingleChoiceItems(mListAdapter,
                    getSelectedItemPosition(), this).show();
        }
    }

    private static class DropDownAdapter implements ListAdapter, SpinnerAdapter {
        private SpinnerAdapter mAdapter;
        private ListAdapter mListAdapter;

        public DropDownAdapter(SpinnerAdapter adapter) {
            mAdapter = adapter;
            if (adapter instanceof ListAdapter) {
                mListAdapter = (ListAdapter) adapter;
            }
        }

        @Override
        public boolean areAllItemsEnabled() {
            final ListAdapter adapter = mListAdapter;
            if (adapter != null) {
                return adapter.areAllItemsEnabled();
            } else {
                return true;
            }
        }

        @Override
        public int getCount() {
            return mAdapter == null ? 0 : mAdapter.getCount();
        }

        @Override
        public View getDropDownView(int position, View convertView,
                ViewGroup parent) {
            return mAdapter == null ? null : mAdapter.getDropDownView(position,
                    convertView, parent);
        }

        @Override
        public Object getItem(int position) {
            return mAdapter == null ? null : mAdapter.getItem(position);
        }

        @Override
        public long getItemId(int position) {
            return mAdapter == null ? -1 : mAdapter.getItemId(position);
        }

        @Override
        public int getItemViewType(int position) {
            return 0;
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            return getDropDownView(position, convertView, parent);
        }

        @Override
        public int getViewTypeCount() {
            return 1;
        }

        @Override
        public boolean hasStableIds() {
            return mAdapter != null && mAdapter.hasStableIds();
        }

        @Override
        public boolean isEmpty() {
            return getCount() == 0;
        }

        @Override
        public boolean isEnabled(int position) {
            final ListAdapter adapter = mListAdapter;
            if (adapter != null) {
                return adapter.isEnabled(position);
            } else {
                return true;
            }
        }

        @Override
        public void registerDataSetObserver(DataSetObserver observer) {
            if (mAdapter != null) {
                mAdapter.registerDataSetObserver(observer);
            }
        }

        @Override
        public void unregisterDataSetObserver(DataSetObserver observer) {
            if (mAdapter != null) {
                mAdapter.unregisterDataSetObserver(observer);
            }
        }
    }

    private class DropdownPopup extends ListPopupWindow implements SpinnerPopup,
            PopupWindow.OnDismissListener {
        private ListAdapter mAdapter;
        private CharSequence mHintText;

        private boolean mSelectionSetted = false;

        public DropdownPopup(Context context, AttributeSet attrs, int defStyle) {
            super(context, attrs, R.attr.listPopupWindowStyle);
            setAnchorView(Spinner.this);
            setModal(true);
            setPromptPosition(ListPopupWindow.POSITION_PROMPT_ABOVE);
            setOnItemClickListener(new android.widget.AdapterView.OnItemClickListener() {
                @Override
                public void onItemClick(android.widget.AdapterView<?> parent,
                        final View v, final int position, long id) {
                    Spinner.this.setSelection(position);
                    if (mOnItemClickListener != null) {
                        Spinner.this.performItemClick(v, position,
                                mAdapter.getItemId(position));
                    }
                    dismiss();
                }
            });
        }

        @Override
        public CharSequence getHintText() {
            return mHintText;
        }

        @Override
        public void onDismiss() {
            mSelectionSetted = false;
        }

        @Override
        public void setAdapter(ListAdapter adapter) {
            super.setAdapter(adapter);
            mAdapter = adapter;
        }

        @Override
        public void setPromptText(CharSequence hintText) {
            mHintText = hintText;
        }

        @Override
        public void show() {
            final Drawable background = getBackground();
            int bgOffset = 0;
            if (background != null) {
                background.getPadding(mTempRect);
                bgOffset = -mTempRect.left;
            } else {
                mTempRect.left = mTempRect.right = 0;
            }
            final int spinnerPaddingLeft = getPaddingLeft();
            if (mDropDownWidth == LayoutParams.WRAP_CONTENT) {
                final int spinnerWidth = Spinner.this.getWidth();
                final int spinnerPaddingRight = getPaddingRight();
                int contentWidth = measureContentWidth(
                        (SpinnerAdapter) mAdapter, getBackground());
                final int contentWidthLimit = getContext().getResources()
                        .getDisplayMetrics().widthPixels
                        - mTempRect.left
                        - mTempRect.right;
                if (contentWidth > contentWidthLimit) {
                    contentWidth = contentWidthLimit;
                }
                setContentWidth(Math.max(contentWidth, spinnerWidth
                        - spinnerPaddingLeft - spinnerPaddingRight));
            } else if (mDropDownWidth == LayoutParams.MATCH_PARENT) {
                final int spinnerWidth = Spinner.this.getWidth();
                final int spinnerPaddingRight = getPaddingRight();
                setContentWidth(spinnerWidth - spinnerPaddingLeft
                        - spinnerPaddingRight);
            } else {
                setContentWidth(mDropDownWidth);
            }
            setHorizontalOffset(bgOffset + spinnerPaddingLeft);
            setInputMethodMode(ListPopupWindow.INPUT_METHOD_NOT_NEEDED);
            super.show();
            getListView().setChoiceMode(AbsListView.CHOICE_MODE_SINGLE);
            if (!mSelectionSetted) {
                mSelectionSetted = true;
                setSelection(Spinner.this.getSelectedItemPosition());
            }
            setOnDismissListener(this);
        }
    }

    private interface SpinnerPopup {
        public void dismiss();

        public Drawable getBackground();

        public CharSequence getHintText();

        public int getHorizontalOffset();

        public int getVerticalOffset();

        public boolean isShowing();

        public void setAdapter(ListAdapter adapter);

        public void setBackgroundDrawable(Drawable bg);

        public void setHorizontalOffset(int px);

        public void setPromptText(CharSequence hintText);

        public void setVerticalOffset(int px);

        public void show();
    }

    private static final int MAX_ITEMS_MEASURED = 15;
    public static final int MODE_DIALOG = 0;
    public static final int MODE_DROPDOWN = 1;
    private static final int MODE_THEME = -1;
    private static final String TAG = "Spinner";
    private boolean mDisableChildrenWhenDisabled;
    int mDropDownWidth;
    private int mGravity;

    private SpinnerPopup mPopup;

    private DropDownAdapter mTempAdapter;

    private Rect mTempRect = new Rect();

    public Spinner(Context context) {
        this(context, null);
    }

    public Spinner(Context context, AttributeSet attrs) {
        this(context, attrs, R.attr.spinnerStyle);
    }

    public Spinner(Context context, AttributeSet attrs, int defStyle) {
        this(context, attrs, defStyle, Spinner.MODE_THEME);
    }

    public Spinner(Context context, AttributeSet attrs, int defStyle, int mode) {
        super(context, attrs, defStyle);
        TypedArray a = context.obtainStyledAttributes(attrs,
                R.styleable.Spinner, defStyle, R.style.Holo_Spinner);
        if (mode == Spinner.MODE_THEME) {
            mode = a.getInt(R.styleable.Spinner_spinnerMode,
                    Spinner.MODE_DIALOG);
        }
        switch (mode) {
            case MODE_DIALOG: {
                mPopup = new DialogPopup();
                break;
            }
            case MODE_DROPDOWN: {
                DropdownPopup popup = new DropdownPopup(context, attrs, defStyle);
                mDropDownWidth = a.getLayoutDimension(
                        R.styleable.Spinner_android_dropDownWidth,
                        ViewGroup.LayoutParams.WRAP_CONTENT);
                popup.setBackgroundDrawable(a
                        .getDrawable(R.styleable.Spinner_android_popupBackground));
                final int verticalOffset = a.getDimensionPixelOffset(
                        R.styleable.Spinner_dropDownVerticalOffset, 0);
                if (verticalOffset != 0) {
                    popup.setVerticalOffset(verticalOffset);
                }
                final int horizontalOffset = a.getDimensionPixelOffset(
                        R.styleable.Spinner_dropDownHorizontalOffset, 0);
                if (horizontalOffset != 0) {
                    popup.setHorizontalOffset(horizontalOffset);
                }
                mPopup = popup;
                break;

            }
        }
        mGravity = a
                .getInt(R.styleable.Spinner_android_gravity, Gravity.CENTER);
        mPopup.setPromptText(a.getString(R.styleable.Spinner_android_prompt));
        mDisableChildrenWhenDisabled = a.getBoolean(
                R.styleable.Spinner_disableChildrenWhenDisabled, false);
        a.recycle();
        if (mTempAdapter != null) {
            mPopup.setAdapter(mTempAdapter);
            mTempAdapter = null;
        }
    }

    public Spinner(Context context, int mode) {
        this(context, null, R.attr.spinnerStyle, mode);
    }

    @Override
    public int getBaseline() {
        View child = null;
        if (getChildCount() > 0) {
            child = getChildAt(0);
        } else if (getAdapter() != null && getAdapter().getCount() > 0) {
            child = makeAndAddView(0);
            mRecycler.put(0, child);
            removeAllViewsInLayout();
        }
        if (child != null) {
            final int childBaseline = child.getBaseline();
            return childBaseline >= 0 ? child.getTop() + childBaseline : -1;
        } else {
            return -1;
        }
    }

    public int getDropDownHorizontalOffset() {
        return mPopup.getHorizontalOffset();
    }

    public int getDropDownVerticalOffset() {
        return mPopup.getVerticalOffset();
    }

    public int getDropDownWidth() {
        return mDropDownWidth;
    }

    public int getGravity() {
        return mGravity;
    }

    public Drawable getPopupBackground() {
        return mPopup.getBackground();
    }

    public CharSequence getPrompt() {
        return mPopup.getHintText();
    }

    public void internalSetOnItemClickListener(OnItemClickListener l) {
        super.setOnItemClickListener(l);
    }

    @Override
    public void layout(int delta, boolean animate) {
        int childrenLeft = mSpinnerPadding.left;
        int childrenWidth = getRight() - getLeft() - mSpinnerPadding.left
                - mSpinnerPadding.right;
        if (mDataChanged) {
            handleDataChanged();
        }
        if (mItemCount == 0) {
            resetList();
            return;
        }
        if (mNextSelectedPosition >= 0) {
            setSelectedPositionInt(mNextSelectedPosition);
        }
        recycleAllViews();
        removeAllViewsInLayout();
        mFirstPosition = mSelectedPosition;
        View sel = makeAndAddView(mSelectedPosition);
        int width = sel.getMeasuredWidth();
        int selectedOffset = childrenLeft;
        switch (mGravity & Gravity.HORIZONTAL_GRAVITY_MASK) {
            case Gravity.CENTER_HORIZONTAL:
                selectedOffset = childrenLeft + childrenWidth / 2 - width / 2;
                break;
            case Gravity.RIGHT:
                selectedOffset = childrenLeft + childrenWidth - width;
                break;
        }
        sel.offsetLeftAndRight(selectedOffset);
        mRecycler.clear();
        invalidate();
        checkSelectionChanged();
        mDataChanged = false;
        mNeedSync = false;
        setNextSelectedPositionInt(mSelectedPosition);
    }

    private View makeAndAddView(int position) {
        View child;
        if (!mDataChanged) {
            child = mRecycler.get(position);
            if (child != null) {
                setUpChild(child);
                return child;
            }
        }
        child = mAdapter.getView(position, null, this);
        setUpChild(child);
        return child;
    }

    int measureContentWidth(SpinnerAdapter adapter, Drawable background) {
        if (adapter == null) {
            return 0;
        }
        int width = 0;
        View itemView = null;
        int itemType = 0;
        final int widthMeasureSpec = MeasureSpec.makeMeasureSpec(0,
                MeasureSpec.UNSPECIFIED);
        final int heightMeasureSpec = MeasureSpec.makeMeasureSpec(0,
                MeasureSpec.UNSPECIFIED);
        int start = Math.max(0, getSelectedItemPosition());
        final int end = Math.min(adapter.getCount(), start
                + Spinner.MAX_ITEMS_MEASURED);
        final int count = end - start;
        start = Math.max(0, start - (Spinner.MAX_ITEMS_MEASURED - count));
        for (int i = start; i < end; i++) {
            final int positionType = adapter.getItemViewType(i);
            if (positionType != itemType) {
                itemType = positionType;
                itemView = null;
            }
            itemView = adapter.getView(i, itemView, this);
            if (itemView.getLayoutParams() == null) {
                itemView.setLayoutParams(new ViewGroup.LayoutParams(
                        ViewGroup.LayoutParams.WRAP_CONTENT,
                        ViewGroup.LayoutParams.WRAP_CONTENT));
            }
            itemView.measure(widthMeasureSpec, heightMeasureSpec);
            width = Math.max(width, itemView.getMeasuredWidth());
        }
        if (background != null) {
            background.getPadding(mTempRect);
            width += mTempRect.left + mTempRect.right;
        }
        return width;
    }

    @Override
    public void onClick(DialogInterface dialog, int which) {
        setSelection(which);
        dialog.dismiss();
    }

    @Override
    protected void onDetachedFromWindow() {
        super.onDetachedFromWindow();
        if (mPopup != null && mPopup.isShowing()) {
            mPopup.dismiss();
        }
    }

    @Override
    public void onInitializeAccessibilityEvent(AccessibilityEvent event) {
        super.onInitializeAccessibilityEvent(event);
        event.setClassName(Spinner.class.getName());
    }

    @Override
    @SuppressLint("NewApi")
    public void onInitializeAccessibilityNodeInfo(AccessibilityNodeInfo info) {
        super.onInitializeAccessibilityNodeInfo(info);
        info.setClassName(Spinner.class.getName());
    }

    @Override
    protected void onLayout(boolean changed, int l, int t, int r, int b) {
        super.onLayout(changed, l, t, r, b);
        layout(0, false);
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
        if (mPopup != null
                && MeasureSpec.getMode(widthMeasureSpec) == MeasureSpec.AT_MOST) {
            final int measuredWidth = getMeasuredWidth();
            setMeasuredDimension(
                    Math.min(
                            Math.max(
                                    measuredWidth,
                                    measureContentWidth(getAdapter(),
                                            getBackground())), MeasureSpec
                                    .getSize(widthMeasureSpec)),
                    getMeasuredHeight());
        }
    }

    @Override
    public boolean performClick() {
        boolean handled = super.performClick();
        if (!handled) {
            handled = true;
            if (!mPopup.isShowing()) {
                mPopup.show();
            }
        }
        return handled;
    }

    @Override
    public void setAdapter(SpinnerAdapter adapter) {
        super.setAdapter(adapter);
        if (mPopup != null) {
            mPopup.setAdapter(new DropDownAdapter(adapter));
        } else {
            mTempAdapter = new DropDownAdapter(adapter);
        }
    }

    public void setDropDownHorizontalOffset(int pixels) {
        mPopup.setHorizontalOffset(pixels);
    }

    public void setDropDownVerticalOffset(int pixels) {
        mPopup.setVerticalOffset(pixels);
    }

    public void setDropDownWidth(int pixels) {
        if (!(mPopup instanceof DropdownPopup)) {
            Log.e(Spinner.TAG,
                    "Cannot set dropdown width for MODE_DIALOG, ignoring");
            return;
        }
        mDropDownWidth = pixels;
    }

    @Override
    public void setEnabled(boolean enabled) {
        super.setEnabled(enabled);
        if (mDisableChildrenWhenDisabled) {
            final int count = getChildCount();
            for (int i = 0; i < count; i++) {
                getChildAt(i).setEnabled(enabled);
            }
        }
    }

    public void setGravity(int gravity) {
        if (mGravity != gravity) {
            if ((gravity & Gravity.HORIZONTAL_GRAVITY_MASK) == 0) {
                gravity |= Gravity.LEFT;
            }
            mGravity = gravity;
            requestLayout();
        }
    }

    @Override
    public void setOnItemClickListener(OnItemClickListener l) {
        throw new RuntimeException(
                "setOnItemClickListener cannot be used with a spinner.");
    }

    public void setPopupBackgroundDrawable(Drawable background) {
        if (!(mPopup instanceof DropdownPopup)) {
            Log.e(Spinner.TAG,
                    "setPopupBackgroundDrawable: incompatible spinner mode; ignoring...");
            return;
        }
        ((DropdownPopup) mPopup).setBackgroundDrawable(background);
    }

    public void setPopupBackgroundResource(int resId) {
        setPopupBackgroundDrawable(getContext().getResources().getDrawable(
                resId));
    }

    public void setPrompt(CharSequence prompt) {
        mPopup.setPromptText(prompt);
    }

    public void setPromptId(int promptId) {
        setPrompt(getContext().getText(promptId));
    }

    private void setUpChild(View child) {
        ViewGroup.LayoutParams lp = child.getLayoutParams();
        if (lp == null) {
            lp = generateDefaultLayoutParams();
        }
        addViewInLayout(child, 0, lp);
        child.setSelected(hasFocus());
        if (mDisableChildrenWhenDisabled) {
            child.setEnabled(isEnabled());
        }
        int childHeightSpec = ViewGroup.getChildMeasureSpec(mHeightMeasureSpec,
                mSpinnerPadding.top + mSpinnerPadding.bottom, lp.height);
        int childWidthSpec = ViewGroup.getChildMeasureSpec(mWidthMeasureSpec,
                mSpinnerPadding.left + mSpinnerPadding.right, lp.width);
        child.measure(childWidthSpec, childHeightSpec);
        int childLeft = 0;
        int childTop = mSpinnerPadding.top
                + (getMeasuredHeight() - mSpinnerPadding.bottom
                        - mSpinnerPadding.top - child.getMeasuredHeight()) / 2;
        int childBottom = childTop + child.getMeasuredHeight();
        int width = child.getMeasuredWidth();
        int childRight = childLeft + width;
        child.layout(childLeft, childTop, childRight, childBottom);
    }
}
