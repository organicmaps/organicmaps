
package org.holoeverywhere.widget;

import org.holoeverywhere.internal._ViewGroup;
import org.holoeverywhere.util.ReflectHelper;

import android.annotation.SuppressLint;
import android.content.Context;
import android.database.DataSetObserver;
import android.os.Build.VERSION;
import android.os.Parcelable;
import android.os.SystemClock;
import android.util.AttributeSet;
import android.util.SparseArray;
import android.view.SoundEffectConstants;
import android.view.View;
import android.view.ViewDebug;
import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityNodeInfo;
import android.widget.Adapter;

public abstract class AdapterView<T extends Adapter> extends _ViewGroup {
    class AdapterDataSetObserver extends DataSetObserver {
        private Parcelable mInstanceState = null;

        public void clearSavedState() {
            mInstanceState = null;
        }

        @Override
        public void onChanged() {
            mDataChanged = true;
            mOldItemCount = mItemCount;
            mItemCount = getAdapter().getCount();
            if (AdapterView.this.getAdapter().hasStableIds()
                    && mInstanceState != null && mOldItemCount == 0
                    && mItemCount > 0) {
                onRestoreInstanceState(mInstanceState);
                mInstanceState = null;
            } else {
                rememberSyncState();
            }
            checkFocus();
            requestLayout();
        }

        @Override
        public void onInvalidated() {
            mDataChanged = true;
            if (AdapterView.this.getAdapter().hasStableIds()) {
                mInstanceState = onSaveInstanceState();
            }
            mOldItemCount = mItemCount;
            mItemCount = 0;
            mSelectedPosition = AdapterView.INVALID_POSITION;
            mSelectedRowId = AdapterView.INVALID_ROW_ID;
            mNextSelectedPosition = AdapterView.INVALID_POSITION;
            mNextSelectedRowId = AdapterView.INVALID_ROW_ID;
            mNeedSync = false;
            checkFocus();
            requestLayout();
        }
    }

    public interface OnItemClickListener {
        void onItemClick(AdapterView<?> parent, View view, int position, long id);
    }

    public interface OnItemLongClickListener {
        boolean onItemLongClick(AdapterView<?> parent, View view, int position,
                long id);
    }

    public interface OnItemSelectedListener {
        void onItemSelected(AdapterView<?> parent, View view, int position,
                long id);

        void onNothingSelected(AdapterView<?> parent);
    }

    private class SelectionNotifier implements Runnable {
        @Override
        public void run() {
            if (mDataChanged) {
                if (getAdapter() != null) {
                    post(this);
                }
            } else {
                fireOnSelected();
                performAccessibilityActionsOnSelected();
            }
        }
    }

    public static final int INVALID_POSITION = -1;
    public static final long INVALID_ROW_ID = Long.MIN_VALUE;
    public static final int ITEM_VIEW_TYPE_HEADER_OR_FOOTER = -2;
    public static final int ITEM_VIEW_TYPE_IGNORE = -1;
    static final int SYNC_FIRST_POSITION = 1;
    static final int SYNC_MAX_DURATION_MILLIS = 100;
    static final int SYNC_SELECTED_POSITION = 0;
    boolean mBlockLayoutRequests = false;
    boolean mDataChanged;
    private boolean mDesiredFocusableInTouchModeState;
    private boolean mDesiredFocusableState;
    private View mEmptyView;
    @ViewDebug.ExportedProperty(category = "scrolling")
    int mFirstPosition = 0;
    boolean mInLayout = false;
    @ViewDebug.ExportedProperty(category = "list")
    int mItemCount;
    private int mLayoutHeight;
    boolean mNeedSync = false;
    @ViewDebug.ExportedProperty(category = "list")
    int mNextSelectedPosition = AdapterView.INVALID_POSITION;
    long mNextSelectedRowId = AdapterView.INVALID_ROW_ID;
    int mOldItemCount;
    int mOldSelectedPosition = AdapterView.INVALID_POSITION;
    long mOldSelectedRowId = AdapterView.INVALID_ROW_ID;
    OnItemClickListener mOnItemClickListener;
    OnItemLongClickListener mOnItemLongClickListener;
    OnItemSelectedListener mOnItemSelectedListener;
    @ViewDebug.ExportedProperty(category = "list")
    int mSelectedPosition = AdapterView.INVALID_POSITION;
    long mSelectedRowId = AdapterView.INVALID_ROW_ID;

    private SelectionNotifier mSelectionNotifier;

    int mSpecificTop;

    long mSyncHeight;

    int mSyncMode;

    int mSyncPosition;

    long mSyncRowId = AdapterView.INVALID_ROW_ID;

    public AdapterView(Context context) {
        super(context);
    }

    public AdapterView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @SuppressLint("NewApi")
    public AdapterView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        if (VERSION.SDK_INT >= 16
                && getImportantForAccessibility() == View.IMPORTANT_FOR_ACCESSIBILITY_AUTO) {
            setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_YES);
        }
    }

    @Override
    public void addView(View child) {
        throw new UnsupportedOperationException(
                "addView(View) is not supported in AdapterView");
    }

    @Override
    public void addView(View child, int index) {
        throw new UnsupportedOperationException(
                "addView(View, int) is not supported in AdapterView");
    }

    @Override
    public void addView(View child, int index, LayoutParams params) {
        throw new UnsupportedOperationException(
                "addView(View, int, LayoutParams) "
                        + "is not supported in AdapterView");
    }

    @Override
    public void addView(View child, LayoutParams params) {
        throw new UnsupportedOperationException("addView(View, LayoutParams) "
                + "is not supported in AdapterView");
    }

    @Override
    protected boolean canAnimate() {
        return super.canAnimate() && mItemCount > 0;
    }

    void checkFocus() {
        final T adapter = getAdapter();
        final boolean empty = adapter == null || adapter.getCount() == 0;
        final boolean focusable = !empty || isInFilterMode();
        super.setFocusableInTouchMode(focusable
                && mDesiredFocusableInTouchModeState);
        super.setFocusable(focusable && mDesiredFocusableState);
        if (mEmptyView != null) {
            updateEmptyStatus(adapter == null || adapter.isEmpty());
        }
    }

    void checkSelectionChanged() {
        if (mSelectedPosition != mOldSelectedPosition
                || mSelectedRowId != mOldSelectedRowId) {
            selectionChanged();
            mOldSelectedPosition = mSelectedPosition;
            mOldSelectedRowId = mSelectedRowId;
        }
    }

    @Override
    public boolean dispatchPopulateAccessibilityEvent(AccessibilityEvent event) {
        View selectedView = getSelectedView();
        if (selectedView != null
                && selectedView.getVisibility() == View.VISIBLE
                && selectedView.dispatchPopulateAccessibilityEvent(event)) {
            return true;
        }
        return false;
    }

    @Override
    protected void dispatchRestoreInstanceState(
            SparseArray<Parcelable> container) {
        dispatchThawSelfOnly(container);
    }

    @Override
    protected void dispatchSaveInstanceState(SparseArray<Parcelable> container) {
        dispatchFreezeSelfOnly(container);
    }

    int findSyncPosition() {
        int count = mItemCount;
        if (count == 0) {
            return AdapterView.INVALID_POSITION;
        }
        long idToMatch = mSyncRowId;
        int seed = mSyncPosition;
        if (idToMatch == AdapterView.INVALID_ROW_ID) {
            return AdapterView.INVALID_POSITION;
        }
        seed = Math.max(0, seed);
        seed = Math.min(count - 1, seed);
        long endTime = SystemClock.uptimeMillis()
                + AdapterView.SYNC_MAX_DURATION_MILLIS;
        long rowId;
        int first = seed;
        int last = seed;
        boolean next = false;
        boolean hitFirst;
        boolean hitLast;
        T adapter = getAdapter();
        if (adapter == null) {
            return AdapterView.INVALID_POSITION;
        }
        while (SystemClock.uptimeMillis() <= endTime) {
            rowId = adapter.getItemId(seed);
            if (rowId == idToMatch) {
                return seed;
            }
            hitLast = last == count - 1;
            hitFirst = first == 0;
            if (hitLast && hitFirst) {
                break;
            }
            if (hitFirst || next && !hitLast) {
                last++;
                seed = last;
                next = false;
            } else if (hitLast || !next && !hitFirst) {
                first--;
                seed = first;
                next = true;
            }
        }
        return AdapterView.INVALID_POSITION;
    }

    private void fireOnSelected() {
        if (mOnItemSelectedListener == null) {
            return;
        }
        final int selection = getSelectedItemPosition();
        if (selection >= 0) {
            View v = getSelectedView();
            mOnItemSelectedListener.onItemSelected(this, v, selection,
                    getAdapter().getItemId(selection));
        } else {
            mOnItemSelectedListener.onNothingSelected(this);
        }
    }

    public abstract T getAdapter();

    @ViewDebug.CapturedViewProperty
    public int getCount() {
        return mItemCount;
    }

    public View getEmptyView() {
        return mEmptyView;
    }

    public int getFirstVisiblePosition() {
        return mFirstPosition;
    }

    public Object getItemAtPosition(int position) {
        T adapter = getAdapter();
        return adapter == null || position < 0 ? null : adapter
                .getItem(position);
    }

    public long getItemIdAtPosition(int position) {
        T adapter = getAdapter();
        return adapter == null || position < 0 ? AdapterView.INVALID_ROW_ID
                : adapter.getItemId(position);
    }

    public int getLastVisiblePosition() {
        return mFirstPosition + getChildCount() - 1;
    }

    public final OnItemClickListener getOnItemClickListener() {
        return mOnItemClickListener;
    }

    public final OnItemLongClickListener getOnItemLongClickListener() {
        return mOnItemLongClickListener;
    }

    public final OnItemSelectedListener getOnItemSelectedListener() {
        return mOnItemSelectedListener;
    }

    public int getPositionForView(View view) {
        View listItem = view;
        try {
            View v;
            while (!(v = (View) listItem.getParent()).equals(this)) {
                listItem = v;
            }
        } catch (ClassCastException e) {
            return AdapterView.INVALID_POSITION;
        }
        final int childCount = getChildCount();
        for (int i = 0; i < childCount; i++) {
            if (getChildAt(i).equals(listItem)) {
                return mFirstPosition + i;
            }
        }
        return AdapterView.INVALID_POSITION;
    }

    public Object getSelectedItem() {
        T adapter = getAdapter();
        int selection = getSelectedItemPosition();
        if (adapter != null && adapter.getCount() > 0 && selection >= 0) {
            return adapter.getItem(selection);
        } else {
            return null;
        }
    }

    @ViewDebug.CapturedViewProperty
    public long getSelectedItemId() {
        return mNextSelectedRowId;
    }

    @ViewDebug.CapturedViewProperty
    public int getSelectedItemPosition() {
        return mNextSelectedPosition;
    }

    public abstract View getSelectedView();

    void handleDataChanged() {
        final int count = mItemCount;
        boolean found = false;
        if (count > 0) {
            int newPos;
            if (mNeedSync) {
                mNeedSync = false;
                newPos = findSyncPosition();
                if (newPos >= 0) {
                    int selectablePos = lookForSelectablePosition(newPos, true);
                    if (selectablePos == newPos) {
                        setNextSelectedPositionInt(newPos);
                        found = true;
                    }
                }
            }
            if (!found) {
                newPos = getSelectedItemPosition();
                if (newPos >= count) {
                    newPos = count - 1;
                }
                if (newPos < 0) {
                    newPos = 0;
                }
                int selectablePos = lookForSelectablePosition(newPos, true);
                if (selectablePos < 0) {
                    selectablePos = lookForSelectablePosition(newPos, false);
                }
                if (selectablePos >= 0) {
                    setNextSelectedPositionInt(selectablePos);
                    checkSelectionChanged();
                    found = true;
                }
            }
        }
        if (!found) {
            mSelectedPosition = AdapterView.INVALID_POSITION;
            mSelectedRowId = AdapterView.INVALID_ROW_ID;
            mNextSelectedPosition = AdapterView.INVALID_POSITION;
            mNextSelectedRowId = AdapterView.INVALID_ROW_ID;
            mNeedSync = false;
            checkSelectionChanged();
        }
        ReflectHelper.invoke(this, "notifyAccessibilityStateChanged", null);
    }

    boolean isInFilterMode() {
        return false;
    }

    private boolean isScrollableForAccessibility() {
        T adapter = getAdapter();
        if (adapter != null) {
            final int itemCount = adapter.getCount();
            return itemCount > 0
                    && (getFirstVisiblePosition() > 0 || getLastVisiblePosition() < itemCount - 1);
        }
        return false;
    }

    int lookForSelectablePosition(int position, boolean lookDown) {
        return position;
    }

    @Override
    protected void onDetachedFromWindow() {
        super.onDetachedFromWindow();
        removeCallbacks(mSelectionNotifier);
    }

    @SuppressLint("NewApi")
    @Override
    public void onInitializeAccessibilityEvent(AccessibilityEvent event) {
        super.onInitializeAccessibilityEvent(event);
        event.setClassName(AdapterView.class.getName());
        event.setScrollable(isScrollableForAccessibility());
        View selectedView = getSelectedView();
        if (selectedView != null) {
            event.setEnabled(selectedView.isEnabled());
        }
        event.setCurrentItemIndex(getSelectedItemPosition());
        event.setFromIndex(getFirstVisiblePosition());
        event.setToIndex(getLastVisiblePosition());
        event.setItemCount(getCount());
    }

    @SuppressLint("NewApi")
    @Override
    public void onInitializeAccessibilityNodeInfo(AccessibilityNodeInfo info) {
        super.onInitializeAccessibilityNodeInfo(info);
        info.setClassName(AdapterView.class.getName());
        info.setScrollable(isScrollableForAccessibility());
        View selectedView = getSelectedView();
        if (selectedView != null) {
            info.setEnabled(selectedView.isEnabled());
        }
    }

    @Override
    protected void onLayout(boolean changed, int left, int top, int right,
            int bottom) {
        mLayoutHeight = getHeight();
    }

    @SuppressLint("NewApi")
    @Override
    public boolean onRequestSendAccessibilityEvent(View child,
            AccessibilityEvent event) {
        if (super.onRequestSendAccessibilityEvent(child, event)) {
            AccessibilityEvent record = AccessibilityEvent.obtain();
            onInitializeAccessibilityEvent(record);
            child.dispatchPopulateAccessibilityEvent(record);
            event.appendRecord(record);
            return true;
        }
        return false;
    }

    private void performAccessibilityActionsOnSelected() {
        if (!isAccessibilityManagerEnabled()) {
            return;
        }
        final int position = getSelectedItemPosition();
        if (position >= 0) {
            sendAccessibilityEvent(AccessibilityEvent.TYPE_VIEW_SELECTED);
        }
    }

    public boolean performItemClick(View view, int position, long id) {
        if (mOnItemClickListener != null) {
            playSoundEffect(SoundEffectConstants.CLICK);
            if (view != null) {
                view.sendAccessibilityEvent(AccessibilityEvent.TYPE_VIEW_CLICKED);
            }
            mOnItemClickListener.onItemClick(this, view, position, id);
            return true;
        }

        return false;
    }

    void rememberSyncState() {
        if (getChildCount() > 0) {
            mNeedSync = true;
            mSyncHeight = mLayoutHeight;
            if (mSelectedPosition >= 0) {
                View v = getChildAt(mSelectedPosition - mFirstPosition);
                mSyncRowId = mNextSelectedRowId;
                mSyncPosition = mNextSelectedPosition;
                if (v != null) {
                    mSpecificTop = v.getTop();
                }
                mSyncMode = AdapterView.SYNC_SELECTED_POSITION;
            } else {
                View v = getChildAt(0);
                T adapter = getAdapter();
                if (mFirstPosition >= 0 && mFirstPosition < adapter.getCount()) {
                    mSyncRowId = adapter.getItemId(mFirstPosition);
                } else {
                    mSyncRowId = View.NO_ID;
                }
                mSyncPosition = mFirstPosition;
                if (v != null) {
                    mSpecificTop = v.getTop();
                }
                mSyncMode = AdapterView.SYNC_FIRST_POSITION;
            }
        }
    }

    @Override
    public void removeAllViews() {
        throw new UnsupportedOperationException(
                "removeAllViews() is not supported in AdapterView");
    }

    @Override
    public void removeView(View child) {
        throw new UnsupportedOperationException(
                "removeView(View) is not supported in AdapterView");
    }

    @Override
    public void removeViewAt(int index) {
        throw new UnsupportedOperationException(
                "removeViewAt(int) is not supported in AdapterView");
    }

    void selectionChanged() {
        if (mOnItemSelectedListener != null || isAccessibilityManagerEnabled()) {
            if (mInLayout || mBlockLayoutRequests) {
                if (mSelectionNotifier == null) {
                    mSelectionNotifier = new SelectionNotifier();
                }
                post(mSelectionNotifier);
            } else {
                fireOnSelected();
                performAccessibilityActionsOnSelected();
            }
        }
    }

    public abstract void setAdapter(T adapter);

    @SuppressLint("NewApi")
    public void setEmptyView(View emptyView) {
        mEmptyView = emptyView;
        if (VERSION.SDK_INT >= 16
                && emptyView != null
                && emptyView.getImportantForAccessibility() == View.IMPORTANT_FOR_ACCESSIBILITY_AUTO) {
            emptyView
                    .setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_YES);
        }
        final T adapter = getAdapter();
        final boolean empty = adapter == null || adapter.isEmpty();
        updateEmptyStatus(empty);
    }

    @Override
    public void setFocusable(boolean focusable) {
        final T adapter = getAdapter();
        final boolean empty = adapter == null || adapter.getCount() == 0;
        mDesiredFocusableState = focusable;
        if (!focusable) {
            mDesiredFocusableInTouchModeState = false;
        }
        super.setFocusable(focusable && (!empty || isInFilterMode()));
    }

    @Override
    public void setFocusableInTouchMode(boolean focusable) {
        final T adapter = getAdapter();
        final boolean empty = adapter == null || adapter.getCount() == 0;

        mDesiredFocusableInTouchModeState = focusable;
        if (focusable) {
            mDesiredFocusableState = true;
        }

        super.setFocusableInTouchMode(focusable && (!empty || isInFilterMode()));
    }

    void setNextSelectedPositionInt(int position) {
        mNextSelectedPosition = position;
        mNextSelectedRowId = getItemIdAtPosition(position);
        if (mNeedSync && mSyncMode == AdapterView.SYNC_SELECTED_POSITION
                && position >= 0) {
            mSyncPosition = position;
            mSyncRowId = mNextSelectedRowId;
        }
    }

    @Override
    public void setOnClickListener(OnClickListener l) {
        throw new RuntimeException(
                "Don't call setOnClickListener for an AdapterView. "
                        + "You probably want setOnItemClickListener instead");
    }

    public void setOnItemClickListener(OnItemClickListener listener) {
        mOnItemClickListener = listener;
    }

    public void setOnItemLongClickListener(OnItemLongClickListener listener) {
        if (!isLongClickable()) {
            setLongClickable(true);
        }
        mOnItemLongClickListener = listener;
    }

    public void setOnItemSelectedListener(OnItemSelectedListener listener) {
        mOnItemSelectedListener = listener;
    }

    void setSelectedPositionInt(int position) {
        mSelectedPosition = position;
        mSelectedRowId = getItemIdAtPosition(position);
    }

    public abstract void setSelection(int position);

    private void updateEmptyStatus(boolean empty) {
        if (isInFilterMode()) {
            empty = false;
        }
        if (empty) {
            if (mEmptyView != null) {
                mEmptyView.setVisibility(View.VISIBLE);
                setVisibility(View.GONE);
            } else {
                setVisibility(View.VISIBLE);
            }
            if (mDataChanged) {
                this.onLayout(false, getLeft(), getTop(), getRight(),
                        getBottom());
            }
        } else {
            if (mEmptyView != null) {
                mEmptyView.setVisibility(View.GONE);
            }
            setVisibility(View.VISIBLE);
        }
    }
}
