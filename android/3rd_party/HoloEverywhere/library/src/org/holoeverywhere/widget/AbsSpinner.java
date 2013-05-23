
package org.holoeverywhere.widget;

import org.holoeverywhere.ArrayAdapter;
import org.holoeverywhere.R;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.res.TypedArray;
import android.database.DataSetObserver;
import android.graphics.Rect;
import android.os.Build.VERSION;
import android.os.Parcel;
import android.os.Parcelable;
import android.util.AttributeSet;
import android.util.SparseArray;
import android.view.View;
import android.view.ViewGroup;
import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityNodeInfo;
import android.widget.SpinnerAdapter;

public abstract class AbsSpinner extends AdapterView<SpinnerAdapter> {
    class RecycleBin {
        private final SparseArray<View> mScrapHeap = new SparseArray<View>();

        void clear() {
            final SparseArray<View> scrapHeap = mScrapHeap;
            final int count = scrapHeap.size();
            for (int i = 0; i < count; i++) {
                final View view = scrapHeap.valueAt(i);
                if (view != null) {
                    removeDetachedView(view, true);
                }
            }
            scrapHeap.clear();
        }

        View get(int position) {
            View result = mScrapHeap.get(position);
            if (result != null) {
                mScrapHeap.delete(position);
            }
            return result;
        }

        public void put(int position, View v) {
            mScrapHeap.put(position, v);
        }
    }

    static class SavedState extends BaseSavedState {
        public static final Parcelable.Creator<SavedState> CREATOR = new Parcelable.Creator<SavedState>() {
            @Override
            public SavedState createFromParcel(Parcel in) {
                return new SavedState(in);
            }

            @Override
            public SavedState[] newArray(int size) {
                return new SavedState[size];
            }
        };
        int position;

        long selectedId;

        private SavedState(Parcel in) {
            super(in);
            selectedId = in.readLong();
            position = in.readInt();
        }

        SavedState(Parcelable superState) {
            super(superState);
        }

        @Override
        public String toString() {
            return "AbsSpinner.SavedState{"
                    + Integer.toHexString(System.identityHashCode(this))
                    + " selectedId=" + selectedId + " position=" + position
                    + "}";
        }

        @Override
        public void writeToParcel(Parcel out, int flags) {
            super.writeToParcel(out, flags);
            out.writeLong(selectedId);
            out.writeInt(position);
        }
    }

    SpinnerAdapter mAdapter;
    private DataSetObserver mDataSetObserver;
    int mHeightMeasureSpec;
    final RecycleBin mRecycler = new RecycleBin();
    int mSelectionBottomPadding = 0;
    int mSelectionLeftPadding = 0;
    int mSelectionRightPadding = 0;
    int mSelectionTopPadding = 0;
    final Rect mSpinnerPadding = new Rect();

    private Rect mTouchFrame;

    int mWidthMeasureSpec;

    public AbsSpinner(Context context) {
        super(context);
        initAbsSpinner();
    }

    public AbsSpinner(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public AbsSpinner(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        initAbsSpinner();
        TypedArray a = context.obtainStyledAttributes(attrs,
                R.styleable.AbsSpinner, defStyle, 0);
        CharSequence[] entries = a
                .getTextArray(R.styleable.AbsSpinner_android_entries);
        if (entries != null) {
            ArrayAdapter<CharSequence> adapter = new ArrayAdapter<CharSequence>(
                    context, R.layout.simple_spinner_item, entries);
            adapter.setDropDownViewResource(R.layout.simple_spinner_dropdown_item);
            setAdapter(adapter);
        }
        a.recycle();
    }

    @Override
    protected ViewGroup.LayoutParams generateDefaultLayoutParams() {
        return new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT);
    }

    @Override
    public SpinnerAdapter getAdapter() {
        return mAdapter;
    }

    int getChildHeight(View child) {
        return child.getMeasuredHeight();
    }

    int getChildWidth(View child) {
        return child.getMeasuredWidth();
    }

    @Override
    public int getCount() {
        return mItemCount;
    }

    @Override
    public View getSelectedView() {
        if (mItemCount > 0 && mSelectedPosition >= 0) {
            return getChildAt(mSelectedPosition - mFirstPosition);
        } else {
            return null;
        }
    }

    private void initAbsSpinner() {
        setFocusable(true);
        setWillNotDraw(false);
    }

    abstract void layout(int delta, boolean animate);

    @Override
    @SuppressLint("NewApi")
    public void onInitializeAccessibilityEvent(AccessibilityEvent event) {
        super.onInitializeAccessibilityEvent(event);
        event.setClassName(AbsSpinner.class.getName());
    }

    @Override
    @SuppressLint("NewApi")
    public void onInitializeAccessibilityNodeInfo(AccessibilityNodeInfo info) {
        super.onInitializeAccessibilityNodeInfo(info);
        info.setClassName(AbsSpinner.class.getName());
    }

    @SuppressLint("NewApi")
    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        int widthMode = MeasureSpec.getMode(widthMeasureSpec);
        int widthSize;
        int heightSize;
        mSpinnerPadding.left = getPaddingLeft() > mSelectionLeftPadding ? getPaddingLeft()
                : mSelectionLeftPadding;
        mSpinnerPadding.top = getPaddingTop() > mSelectionTopPadding ? getPaddingTop()
                : mSelectionTopPadding;
        mSpinnerPadding.right = getPaddingRight() > mSelectionRightPadding ? getPaddingRight()
                : mSelectionRightPadding;
        mSpinnerPadding.bottom = getPaddingBottom() > mSelectionBottomPadding ? getPaddingBottom()
                : mSelectionBottomPadding;
        if (mDataChanged) {
            handleDataChanged();
        }
        int preferredHeight = 0;
        int preferredWidth = 0;
        boolean needsMeasuring = true;
        int selectedPosition = getSelectedItemPosition();
        if (selectedPosition >= 0 && mAdapter != null
                && selectedPosition < mAdapter.getCount()) {
            View view = mRecycler.get(selectedPosition);
            if (view == null) {
                view = mAdapter.getView(selectedPosition, null, this);
                if (VERSION.SDK_INT >= 16
                        && view.getImportantForAccessibility() == View.IMPORTANT_FOR_ACCESSIBILITY_AUTO) {
                    view.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_YES);
                }
            }
            if (view != null) {
                mRecycler.put(selectedPosition, view);
            }
            if (view != null) {
                if (view.getLayoutParams() == null) {
                    mBlockLayoutRequests = true;
                    view.setLayoutParams(generateDefaultLayoutParams());
                    mBlockLayoutRequests = false;
                }
                measureChild(view, widthMeasureSpec, heightMeasureSpec);
                preferredHeight = getChildHeight(view) + mSpinnerPadding.top
                        + mSpinnerPadding.bottom;
                preferredWidth = getChildWidth(view) + mSpinnerPadding.left
                        + mSpinnerPadding.right;
                needsMeasuring = false;
            }
        }
        if (needsMeasuring) {
            preferredHeight = mSpinnerPadding.top + mSpinnerPadding.bottom;
            if (widthMode == MeasureSpec.UNSPECIFIED) {
                preferredWidth = mSpinnerPadding.left + mSpinnerPadding.right;
            }
        }
        preferredHeight = Math
                .max(preferredHeight, getSuggestedMinimumHeight());
        preferredWidth = Math.max(preferredWidth, getSuggestedMinimumWidth());
        heightSize = org.holoeverywhere.internal._View
                .supportResolveSizeAndState(preferredHeight, heightMeasureSpec,
                        0);
        widthSize = org.holoeverywhere.internal._View
                .supportResolveSizeAndState(preferredWidth, widthMeasureSpec, 0);
        setMeasuredDimension(widthSize, heightSize);
        mHeightMeasureSpec = heightMeasureSpec;
        mWidthMeasureSpec = widthMeasureSpec;
    }

    @Override
    public void onRestoreInstanceState(Parcelable state) {
        SavedState ss = (SavedState) state;

        super.onRestoreInstanceState(ss.getSuperState());

        if (ss.selectedId >= 0) {
            mDataChanged = true;
            mNeedSync = true;
            mSyncRowId = ss.selectedId;
            mSyncPosition = ss.position;
            mSyncMode = AdapterView.SYNC_SELECTED_POSITION;
            requestLayout();
        }
    }

    @Override
    public Parcelable onSaveInstanceState() {
        Parcelable superState = super.onSaveInstanceState();
        SavedState ss = new SavedState(superState);
        ss.selectedId = getSelectedItemId();
        if (ss.selectedId >= 0) {
            ss.position = getSelectedItemPosition();
        } else {
            ss.position = AdapterView.INVALID_POSITION;
        }
        return ss;
    }

    public int pointToPosition(int x, int y) {
        Rect frame = mTouchFrame;
        if (frame == null) {
            mTouchFrame = new Rect();
            frame = mTouchFrame;
        }
        final int count = getChildCount();
        for (int i = count - 1; i >= 0; i--) {
            View child = getChildAt(i);
            if (child.getVisibility() == View.VISIBLE) {
                child.getHitRect(frame);
                if (frame.contains(x, y)) {
                    return mFirstPosition + i;
                }
            }
        }
        return AdapterView.INVALID_POSITION;
    }

    void recycleAllViews() {
        final int childCount = getChildCount();
        final AbsSpinner.RecycleBin recycleBin = mRecycler;
        final int position = mFirstPosition;
        for (int i = 0; i < childCount; i++) {
            View v = getChildAt(i);
            int index = position + i;
            recycleBin.put(index, v);
        }
    }

    @Override
    public void requestLayout() {
        if (!mBlockLayoutRequests) {
            super.requestLayout();
        }
    }

    void resetList() {
        mDataChanged = false;
        mNeedSync = false;
        removeAllViewsInLayout();
        mOldSelectedPosition = AdapterView.INVALID_POSITION;
        mOldSelectedRowId = AdapterView.INVALID_ROW_ID;
        setSelectedPositionInt(AdapterView.INVALID_POSITION);
        setNextSelectedPositionInt(AdapterView.INVALID_POSITION);
        invalidate();
    }

    @Override
    public void setAdapter(SpinnerAdapter adapter) {
        if (null != mAdapter) {
            mAdapter.unregisterDataSetObserver(mDataSetObserver);
            resetList();
        }
        mAdapter = adapter;
        mOldSelectedPosition = AdapterView.INVALID_POSITION;
        mOldSelectedRowId = AdapterView.INVALID_ROW_ID;
        if (mAdapter != null) {
            mOldItemCount = mItemCount;
            mItemCount = mAdapter.getCount();
            checkFocus();
            mDataSetObserver = new AdapterDataSetObserver();
            mAdapter.registerDataSetObserver(mDataSetObserver);
            int position = mItemCount > 0 ? 0 : AdapterView.INVALID_POSITION;
            setSelectedPositionInt(position);
            setNextSelectedPositionInt(position);
            if (mItemCount == 0) {
                checkSelectionChanged();
            }
        } else {
            checkFocus();
            resetList();
            checkSelectionChanged();
        }
        requestLayout();
    }

    @Override
    public void setSelection(int position) {
        setNextSelectedPositionInt(position);
        requestLayout();
        invalidate();
    }

    public void setSelection(int position, boolean animate) {
        boolean shouldAnimate = animate && mFirstPosition <= position
                && position <= mFirstPosition + getChildCount() - 1;
        setSelectionInt(position, shouldAnimate);
    }

    void setSelectionInt(int position, boolean animate) {
        if (position != mOldSelectedPosition) {
            mBlockLayoutRequests = true;
            int delta = position - mSelectedPosition;
            setNextSelectedPositionInt(position);
            layout(delta, animate);
            mBlockLayoutRequests = false;
        }
    }
}
