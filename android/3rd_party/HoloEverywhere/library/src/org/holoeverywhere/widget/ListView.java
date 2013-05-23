
package org.holoeverywhere.widget;

import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.List;

import org.holoeverywhere.HoloEverywhere;
import org.holoeverywhere.IHoloActivity.OnWindowFocusChangeListener;
import org.holoeverywhere.R;
import org.holoeverywhere.app.Activity;
import org.holoeverywhere.drawable.DrawableCompat;
import org.holoeverywhere.widget.HeaderViewListAdapter.ViewInfo;
import org.holoeverywhere.widget.ListAdapterWrapper.ListAdapterCallback;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.os.Build.VERSION;
import android.os.Build.VERSION_CODES;
import android.os.Parcel;
import android.os.Parcelable;
import android.support.v4.util.LongSparseArray;
import android.util.AttributeSet;
import android.util.SparseBooleanArray;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.HapticFeedbackConstants;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewDebug.ExportedProperty;
import android.view.ViewGroup;
import android.view.ViewParent;
import android.widget.AbsListView;
import android.widget.AdapterView;
import android.widget.Checkable;
import android.widget.ListAdapter;

import com.actionbarsherlock.internal.view.menu.ContextMenuBuilder.ContextMenuInfoGetter;
import com.actionbarsherlock.view.ActionMode;
import com.actionbarsherlock.view.Menu;
import com.actionbarsherlock.view.MenuItem;

public class ListView extends android.widget.ListView implements OnWindowFocusChangeListener,
        ContextMenuInfoGetter {
    public interface MultiChoiceModeListener extends ActionMode.Callback {
        public void onItemCheckedStateChanged(ActionMode mode, int position,
                long id, boolean checked);
    }

    private final class MultiChoiceModeWrapper implements MultiChoiceModeListener {
        private MultiChoiceModeListener mWrapped;

        @Override
        public boolean onActionItemClicked(ActionMode mode, MenuItem item) {
            return mWrapped.onActionItemClicked(mode, item);
        }

        @Override
        public boolean onCreateActionMode(ActionMode mode, Menu menu) {
            if (mWrapped.onCreateActionMode(mode, menu)) {
                setLongClickable(false);
                return true;
            }
            return false;
        }

        @Override
        public void onDestroyActionMode(ActionMode mode) {
            mWrapped.onDestroyActionMode(mode);
            mChoiceActionMode = null;
            clearChoices();
            updateOnScreenCheckedViews();
            setLongClickable(true);
        }

        @Override
        public void onItemCheckedStateChanged(ActionMode mode,
                int position, long id, boolean checked) {
            mWrapped.onItemCheckedStateChanged(mode, position, id, checked);
            if (getCheckedItemCount() == 0) {
                mode.finish();
            }
        }

        @Override
        public boolean onPrepareActionMode(ActionMode mode, Menu menu) {
            return mWrapped.onPrepareActionMode(mode, menu);
        }

        public void setWrapped(MultiChoiceModeListener wrapped) {
            mWrapped = wrapped;
        }
    }

    private final class OnItemLongClickListenerWrapper implements OnItemLongClickListener {
        private OnItemLongClickListener wrapped;

        @Override
        public boolean onItemLongClick(AdapterView<?> adapterView, View view, int position, long id) {
            return performItemLongClick(view, position, id);
        }

        public void setWrapped(OnItemLongClickListener wrapped) {
            this.wrapped = wrapped;
            if (wrapped != null) {
                setLongClickable(true);
            }
        }
    }

    static final class SavedState extends BaseSavedState {
        public static final Creator<SavedState> CREATOR = new Creator<SavedState>() {
            @Override
            public SavedState createFromParcel(Parcel parcel) {
                return new SavedState(parcel);
            }

            @Override
            public SavedState[] newArray(int size) {
                return new SavedState[size];
            }
        };
        int checkedItemCount;
        LongSparseArray<Integer> checkIdState;
        SparseBooleanArray checkState;
        boolean inActionMode;

        public SavedState(Parcel in) {
            super(in);
            inActionMode = in.readByte() != 0;
            checkedItemCount = in.readInt();
            checkState = in.readSparseBooleanArray();
            final int N = in.readInt();
            if (N > 0) {
                checkIdState = new LongSparseArray<Integer>();
                for (int i = 0; i < N; i++) {
                    final long key = in.readLong();
                    final int value = in.readInt();
                    checkIdState.put(key, value);
                }
            }
        }

        public SavedState(Parcelable superState) {
            super(superState);
        }

        @Override
        public void writeToParcel(Parcel out, int flags) {
            super.writeToParcel(out, flags);
            out.writeByte((byte) (inActionMode ? 1 : 0));
            out.writeInt(checkedItemCount);
            out.writeSparseBooleanArray(checkState);
            final int N = checkIdState != null ? checkIdState.size() : 0;
            out.writeInt(N);
            for (int i = 0; i < N; i++) {
                out.writeLong(checkIdState.keyAt(i));
                out.writeInt(checkIdState.valueAt(i));
            }
        }
    }

    public static final int CHOICE_MODE_MULTIPLE = AbsListView.CHOICE_MODE_MULTIPLE;
    public static final int CHOICE_MODE_MULTIPLE_MODAL = AbsListView.CHOICE_MODE_MULTIPLE_MODAL;
    public static final int CHOICE_MODE_NONE = AbsListView.CHOICE_MODE_NONE;
    public static final int CHOICE_MODE_SINGLE = AbsListView.CHOICE_MODE_SINGLE;
    private static final boolean USE_ACTIVATED = VERSION.SDK_INT >= VERSION_CODES.HONEYCOMB;
    private Activity mActivity;
    private ListAdapterWrapper mAdapter;
    private boolean mAdapterHasStableIds;
    private LongSparseArray<Integer> mCheckedIdStates;
    private int mCheckedItemCount;
    private SparseBooleanArray mCheckStates;
    private ActionMode mChoiceActionMode;
    private int mChoiceMode;
    private ContextMenuInfo mContextMenuInfo;
    private boolean mEnableModalBackgroundWrapper;
    private boolean mFastScrollEnabled;
    private FastScroller mFastScroller;
    private final List<ViewInfo> mFooterViewInfos = new ArrayList<ViewInfo>(),
            mHeaderViewInfos = new ArrayList<ViewInfo>();
    private boolean mForceFastScrollAlwaysVisibleDisable = false;
    private boolean mForceHeaderListAdapter = false;
    private boolean mIsAttached;
    private int mLastScrollState = OnScrollListener.SCROLL_STATE_IDLE;
    private final ListAdapterCallback mListAdapterCallback = new ListAdapterCallback() {
        @Override
        public void onChanged() {
            if (mFastScroller != null) {
                mFastScroller.onSectionsChanged();
            }
        }

        @Override
        public void onInvalidated() {
            if (mFastScroller != null) {
                mFastScroller.onSectionsChanged();
            }
        }

        @Override
        public View onPrepareView(View view, int position) {
            return ListView.this.onPrepareView(view, position);
        }
    };
    private MultiChoiceModeWrapper mMultiChoiceModeCallback;
    private final OnItemLongClickListenerWrapper mOnItemLongClickListenerWrapper;
    private OnScrollListener mOnScrollListener;
    private boolean mPaddingFromScroller = false;
    private int mVerticalScrollbarPosition = SCROLLBAR_POSITION_DEFAULT;

    public ListView(Context context) {
        this(context, null);
    }

    public ListView(Context context, AttributeSet attrs) {
        this(context, attrs, android.R.attr.listViewStyle);
    }

    @SuppressLint("NewApi")
    public ListView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        if (context instanceof Activity) {
            setActivity((Activity) context);
        }
        if (HoloEverywhere.DISABLE_OVERSCROLL_EFFECT && VERSION.SDK_INT >= 9) {
            setOverScrollMode(OVER_SCROLL_NEVER);
        }

        mOnItemLongClickListenerWrapper = new OnItemLongClickListenerWrapper();
        super.setOnItemLongClickListener(mOnItemLongClickListenerWrapper);
        setLongClickable(false);

        if (VERSION.SDK_INT >= VERSION_CODES.HONEYCOMB) {
            super.setFastScrollAlwaysVisible(false);
        }
        super.setFastScrollEnabled(false);
        super.setChoiceMode(CHOICE_MODE_NONE);
        TypedArray a = context.obtainStyledAttributes(attrs, new int[] {
                android.R.attr.fastScrollEnabled,
                android.R.attr.fastScrollAlwaysVisible,
                android.R.attr.choiceMode,
                android.R.attr.overScrollFooter,
                android.R.attr.overScrollHeader
        }, defStyle, R.style.Holo_ListView);
        setFastScrollEnabled(a.getBoolean(0, false));
        setFastScrollAlwaysVisible(a.getBoolean(1, false));
        setChoiceMode(a.getInt(2, CHOICE_MODE_NONE));
        if (!a.hasValue(3) && VERSION.SDK_INT >= VERSION_CODES.GINGERBREAD) {
            super.setOverscrollFooter(null);
        }
        if (!a.hasValue(4) && VERSION.SDK_INT >= VERSION_CODES.GINGERBREAD) {
            super.setOverscrollHeader(null);
        }
        a.recycle();
    }

    @Override
    public void addFooterView(View v) {
        addFooterView(v, null, true);
    }

    @Override
    public void addFooterView(View v, Object data, boolean isSelectable) {
        if (mAdapter != null && !(mAdapter instanceof HeaderViewListAdapter)) {
            throw new IllegalStateException(
                    "Cannot add footer view to list -- setAdapter has already been called.");
        }
        ViewInfo info = new ViewInfo();
        info.view = v;
        info.data = data;
        info.isSelectable = isSelectable;
        mFooterViewInfos.add(info);
        if (mAdapter != null) {
            invalidateViews();
        }
    }

    @Override
    public void addHeaderView(View v) {
        addHeaderView(v, null, true);
    }

    @Override
    public void addHeaderView(View v, Object data, boolean isSelectable) {
        if (mAdapter != null && !(mAdapter instanceof HeaderViewListAdapter)) {
            throw new IllegalStateException(
                    "Cannot add header view to list -- setAdapter has already been called.");
        }
        ViewInfo info = new ViewInfo();
        info.view = v;
        info.data = data;
        info.isSelectable = isSelectable;
        mHeaderViewInfos.add(info);
        if (mAdapter != null) {
            invalidateViews();
        }
    }

    @Override
    public void clearChoices() {
        if (mCheckStates != null) {
            mCheckStates.clear();
        }
        if (mCheckedIdStates != null) {
            mCheckedIdStates.clear();
        }
        mCheckedItemCount = 0;
    }

    protected ContextMenuInfo createContextMenuInfo(View view, int position,
            long id) {
        return new AdapterContextMenuInfo(view, position, id);
    }

    @Override
    public void draw(Canvas canvas) {
        super.draw(canvas);
        if (mFastScroller != null) {
            final int scrollY = getScrollY();
            if (scrollY != 0) {
                int restoreCount = canvas.save();
                canvas.translate(0, scrollY);
                mFastScroller.draw(canvas);
                canvas.restoreToCount(restoreCount);
            } else {
                mFastScroller.draw(canvas);
            }
        }

    }

    /**
     * O_O This method doesn't override super method, but super class invoke it
     * instead of android.widget.ListView.drawDivider. It's fucking magic of
     * dalvik?
     */
    void drawDivider(Canvas canvas, Rect bounds, int childIndex) {
        final Drawable divider = getDivider();
        divider.setBounds(bounds);
        divider.draw(canvas);
    }

    public Activity getActivity() {
        return mActivity;
    }

    public ListAdapter getAdapterSource() {
        return mAdapter == null ? null : mAdapter.getWrappedAdapter();
    }

    @Override
    public int getCheckedItemCount() {
        return mCheckedItemCount;
    }

    @Override
    public long[] getCheckedItemIds() {
        if (mChoiceMode == CHOICE_MODE_NONE || mCheckedIdStates == null || mAdapter == null) {
            return new long[0];
        }
        final LongSparseArray<Integer> idStates = mCheckedIdStates;
        final int count = idStates.size();
        final long[] ids = new long[count];
        for (int i = 0; i < count; i++) {
            ids[i] = idStates.keyAt(i);
        }
        return ids;
    }

    @Override
    public int getCheckedItemPosition() {
        if (mChoiceMode == CHOICE_MODE_SINGLE && mCheckStates != null && mCheckStates.size() == 1) {
            return mCheckStates.keyAt(0);
        }
        return INVALID_POSITION;
    }

    @Override
    public SparseBooleanArray getCheckedItemPositions() {
        if (mChoiceMode != CHOICE_MODE_NONE) {
            return mCheckStates;
        }
        return null;
    }

    @Override
    @Deprecated
    public long[] getCheckItemIds() {
        return getCheckedItemIds();
    }

    @Override
    public int getChoiceMode() {
        return mChoiceMode;
    }

    @Override
    public ContextMenuInfo getContextMenuInfo() {
        return mContextMenuInfo;
    }

    @Override
    public int getFooterViewsCount() {
        return mFooterViewInfos.size();
    }

    @Override
    public int getHeaderViewsCount() {
        return mHeaderViewInfos.size();
    }

    @Override
    public int getVerticalScrollbarPosition() {
        return mVerticalScrollbarPosition;
    }

    @Override
    public int getVerticalScrollbarWidth() {
        mForceFastScrollAlwaysVisibleDisable = true;
        final int superWidth = super.getVerticalScrollbarWidth();
        mForceFastScrollAlwaysVisibleDisable = false;
        if (isFastScrollAlwaysVisible()) {
            return Math.max(superWidth, mFastScroller.getWidth());
        }
        return superWidth;
    }

    void invokeOnItemScrollListener() {
        final int mFirstPosition = getFirstVisiblePosition();
        final int mItemCount = getCount();
        if (mFastScroller != null) {
            mFastScroller.onScroll(this, mFirstPosition, getChildCount(), mItemCount);
        }
        if (mOnScrollListener != null) {
            mOnScrollListener.onScroll(this, mFirstPosition, getChildCount(), mItemCount);
        }
        onScrollChanged(0, 0, 0, 0);
    }

    public boolean isAttached() {
        return mIsAttached;
    }

    @Override
    public boolean isFastScrollAlwaysVisible() {
        if (mForceFastScrollAlwaysVisibleDisable) {
            return false;
        }
        return mFastScrollEnabled && mFastScroller.isAlwaysShowEnabled();
    }

    @Override
    @ExportedProperty
    public boolean isFastScrollEnabled() {
        return mFastScrollEnabled;
    }

    public boolean isForceHeaderListAdapter() {
        return mForceHeaderListAdapter;
    }

    public boolean isInScrollingContainer() {
        ViewParent p = getParent();
        while (p != null && p instanceof ViewGroup) {
            if (VERSION.SDK_INT >= VERSION_CODES.ICE_CREAM_SANDWICH
                    && ((ViewGroup) p).shouldDelayChildPressedState()) {
                return true;
            }
            p = p.getParent();
        }
        return false;
    }

    @Override
    public boolean isItemChecked(int position) {
        if (mChoiceMode != CHOICE_MODE_NONE && mCheckStates != null) {
            return mCheckStates.get(position);
        }
        return false;
    }

    public boolean isPaddingFromScroller() {
        return mPaddingFromScroller;
    }

    protected boolean isVerticalScrollBarHidden() {
        return mFastScroller != null && mFastScroller.isVisible();
    }

    @Override
    protected void onAttachedToWindow() {
        super.onAttachedToWindow();
        mIsAttached = true;
    }

    @Override
    protected void onDetachedFromWindow() {
        super.onDetachedFromWindow();
        mIsAttached = false;
    }

    @Override
    protected void onFocusChanged(boolean gainFocus, int direction, Rect previouslyFocusedRect) {
        super.onFocusChanged(gainFocus, direction, previouslyFocusedRect);
        if (gainFocus && getSelectedItemPosition() < 0 && !isInTouchMode()) {
            if (!mIsAttached && mAdapter != null) {
                updateOnScreenCheckedViews();
            }
        }
    }

    @Override
    public boolean onInterceptTouchEvent(MotionEvent ev) {
        if (!mIsAttached) {
            return false;
        }
        if (mFastScroller != null && mFastScroller.onInterceptTouchEvent(ev)) {
            return true;
        }
        return super.onInterceptTouchEvent(ev);
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        switch (keyCode) {
            case KeyEvent.KEYCODE_DPAD_CENTER:
            case KeyEvent.KEYCODE_ENTER:
                if (!isEnabled()) {
                    return true;
                }
                if (isClickable() && isPressed() &&
                        getSelectedItemPosition() >= 0 && mAdapter != null &&
                        getSelectedItemPosition() < mAdapter.getCount()) {
                    final View view = getChildAt(getSelectedItemPosition()
                            - getFirstVisiblePosition());
                    if (view != null) {
                        performItemClick(view, getSelectedItemPosition(), getSelectedItemId());
                        view.setPressed(false);
                    }
                    setPressed(false);
                    return true;
                }
                break;
        }
        return super.onKeyUp(keyCode, event);
    }

    @Override
    protected void onLayout(boolean changed, int l, int t, int r, int b) {
        final int mOldItemCount = getCount();
        super.onLayout(changed, l, t, r, b);
        final int mItemCount = getCount();
        if (mFastScroller != null && mItemCount != mOldItemCount) {
            mFastScroller.onItemCountChanged(mOldItemCount, mItemCount);
        }
    }

    public View onPrepareView(View view, int position) {
        if (mChoiceMode != CHOICE_MODE_NONE) {
            if (mCheckStates != null) {
                setStateOnView(view, mCheckStates.get(position));
            } else {
                setStateOnView(view, false);
            }
        }
        return view;
    }

    @Override
    public void onRestoreInstanceState(Parcelable state) {
        SavedState ss = (SavedState) state;
        super.onRestoreInstanceState(ss.getSuperState());
        if (ss.checkState != null) {
            mCheckStates = ss.checkState;
        }
        if (ss.checkIdState != null) {
            mCheckedIdStates = ss.checkIdState;
        }
        mCheckedItemCount = ss.checkedItemCount;
        if (ss.inActionMode && mChoiceMode == CHOICE_MODE_MULTIPLE_MODAL
                && mMultiChoiceModeCallback != null) {
            mChoiceActionMode = startActionMode(mMultiChoiceModeCallback);
        }
        requestLayout();
    }

    @Override
    public Parcelable onSaveInstanceState() {
        SavedState ss = new SavedState(super.onSaveInstanceState());
        ss.inActionMode = mChoiceMode == CHOICE_MODE_MULTIPLE_MODAL && mChoiceActionMode != null;
        ss.checkState = mCheckStates;
        ss.checkIdState = mCheckedIdStates;
        ss.checkedItemCount = mCheckedItemCount;
        return ss;
    }

    @Override
    protected void onSizeChanged(int w, int h, int oldw, int oldh) {
        super.onSizeChanged(w, h, oldw, oldh);
        if (mFastScroller != null) {
            mFastScroller.onSizeChanged(w, h, oldw, oldh);
        }
    }

    @Override
    public boolean onTouchEvent(MotionEvent ev) {
        if (!isEnabled()) {
            return isClickable() || isLongClickable();
        }
        if (!mIsAttached) {
            return false;
        }
        if (mFastScroller != null && mFastScroller.onTouchEvent(ev)) {
            return true;
        }
        return super.onTouchEvent(ev);
    }

    @Override
    public void onWindowFocusChanged(boolean hasWindowFocus) {
        super.onWindowFocusChanged(hasWindowFocus);
        if (hasWindowFocus) {
            updateOnScreenCheckedViews();
        }
    }

    @Override
    public boolean performItemClick(View view, int position, long id) {
        boolean handled = false;
        boolean dispatchItemClick = true;
        if (mChoiceMode != CHOICE_MODE_NONE) {
            handled = true;
            boolean checkedStateChanged = false;
            if (mChoiceMode == CHOICE_MODE_MULTIPLE ||
                    mChoiceMode == CHOICE_MODE_MULTIPLE_MODAL && mChoiceActionMode != null) {
                boolean newValue = !mCheckStates.get(position, false);
                mCheckStates.put(position, newValue);
                if (mCheckedIdStates != null && mAdapter.hasStableIds()) {
                    if (newValue) {
                        mCheckedIdStates.put(mAdapter.getItemId(position), position);
                    } else {
                        mCheckedIdStates.delete(mAdapter.getItemId(position));
                    }
                }
                if (newValue) {
                    mCheckedItemCount++;
                } else {
                    mCheckedItemCount--;
                }
                if (mChoiceActionMode != null) {
                    mMultiChoiceModeCallback.onItemCheckedStateChanged(mChoiceActionMode,
                            position, id, newValue);
                    dispatchItemClick = false;
                }
                checkedStateChanged = true;
            } else if (mChoiceMode == CHOICE_MODE_SINGLE) {
                boolean newValue = !mCheckStates.get(position, false);
                if (newValue) {
                    mCheckStates.clear();
                    mCheckStates.put(position, true);
                    if (mCheckedIdStates != null && mAdapter.hasStableIds()) {
                        mCheckedIdStates.clear();
                        mCheckedIdStates.put(mAdapter.getItemId(position), position);
                    }
                    mCheckedItemCount = 1;
                } else if (mCheckStates.size() == 0 || !mCheckStates.valueAt(0)) {
                    mCheckedItemCount = 0;
                }
                checkedStateChanged = true;
            }
            if (checkedStateChanged) {
                updateOnScreenCheckedViews();
            }
        }
        if (dispatchItemClick) {
            handled |= super.performItemClick(view, position, id);
        }
        return handled;
    }

    public boolean performItemLongClick(final View child,
            final int longPressPosition, final long longPressId) {
        if (mChoiceMode == CHOICE_MODE_MULTIPLE_MODAL) {
            if (mChoiceActionMode == null &&
                    (mChoiceActionMode = startActionMode(mMultiChoiceModeCallback)) != null) {
                setItemChecked(longPressPosition, true);
                performHapticFeedback(HapticFeedbackConstants.LONG_PRESS);
            }
            return true;
        }
        boolean handled = false;
        if (mOnItemLongClickListenerWrapper.wrapped != null) {
            handled = mOnItemLongClickListenerWrapper.wrapped.onItemLongClick(ListView.this, child,
                    longPressPosition, longPressId);
        }
        if (!handled) {
            mContextMenuInfo = createContextMenuInfo(child, longPressPosition, longPressId);
            handled = super.showContextMenuForChild(ListView.this);
        }
        if (handled) {
            performHapticFeedback(HapticFeedbackConstants.LONG_PRESS);
        }
        return handled;
    }

    protected void recomputePaddingFromScroller() {
        final int left = getPaddingLeft();
        final int top = getPaddingTop();
        final int right = getPaddingRight();
        final int bottom = getPaddingBottom();
        if (mPaddingFromScroller) {
            final int scrollbarWidth = getVerticalScrollbarWidth();
            switch (mVerticalScrollbarPosition) {
                case SCROLLBAR_POSITION_LEFT:
                    setPadding(scrollbarWidth, top, right, bottom);
                    break;
                case SCROLLBAR_POSITION_RIGHT:
                case SCROLLBAR_POSITION_DEFAULT:
                default:
                    setPadding(left, top, scrollbarWidth, bottom);
                    break;
            }
        } else {
            setPadding(0, top, 0, bottom);
        }
    }

    @Override
    public boolean removeFooterView(View v) {
        if (mFooterViewInfos.size() > 0) {
            boolean result = false;
            if (mAdapter != null && ((HeaderViewListAdapter) mAdapter).removeFooter(v)) {
                invalidateViews();
                result = true;
            }
            removeViewInfo(v, mFooterViewInfos);
            return result;
        }
        return false;
    }

    @Override
    public boolean removeHeaderView(View v) {
        if (mHeaderViewInfos.size() > 0) {
            boolean result = false;
            if (mAdapter != null && ((HeaderViewListAdapter) mAdapter).removeHeader(v)) {
                invalidateViews();
                result = true;
            }
            removeViewInfo(v, mHeaderViewInfos);
            return result;
        }
        return false;
    }

    private void removeViewInfo(View v, List<ViewInfo> where) {
        int len = where.size();
        for (int i = 0; i < len; ++i) {
            ViewInfo info = where.get(i);
            if (info.view == v) {
                where.remove(i);
                break;
            }
        }
    }

    protected void reportScrollStateChange(int newState) {
        if (newState != mLastScrollState) {
            if (mOnScrollListener != null) {
                mLastScrollState = newState;
                mOnScrollListener.onScrollStateChanged(this, newState);
            }
        }
    }

    public final void setActivity(Activity activity) {
        mActivity = activity;
        if (mActivity != null) {
            mActivity.addOnWindowFocusChangeListener(this);
        }
    }

    @Override
    public void setAdapter(ListAdapter adapter) {
        if (adapter == null) {
            mAdapter = null;
        } else if (mForceHeaderListAdapter || mHeaderViewInfos.size() > 0
                || mFooterViewInfos.size() > 0) {
            mAdapter = new HeaderViewListAdapter(mHeaderViewInfos, mFooterViewInfos, adapter,
                    mListAdapterCallback);
        } else {
            mAdapter = new ListAdapterWrapper(adapter, mListAdapterCallback);
        }
        if (mAdapter != null) {
            mAdapterHasStableIds = mAdapter.hasStableIds();
            if (mChoiceMode != CHOICE_MODE_NONE && mAdapterHasStableIds &&
                    mCheckedIdStates == null) {
                mCheckedIdStates = new LongSparseArray<Integer>();
            }
        }
        if (mCheckStates != null) {
            mCheckStates.clear();
        }
        if (mCheckedIdStates != null) {
            mCheckedIdStates.clear();
        }
        super.setAdapter(mAdapter);
    }

    @Override
    public void setChoiceMode(int choiceMode) {
        mChoiceMode = choiceMode;
        if (mChoiceActionMode != null) {
            mChoiceActionMode.finish();
            mChoiceActionMode = null;
        }
        if (mChoiceMode != CHOICE_MODE_NONE) {
            if (mCheckStates == null) {
                mCheckStates = new SparseBooleanArray();
            }
            if (mCheckedIdStates == null && mAdapter != null && mAdapter.hasStableIds()) {
                mCheckedIdStates = new LongSparseArray<Integer>();
            }
            if (mChoiceMode == CHOICE_MODE_MULTIPLE_MODAL) {
                clearChoices();
                setLongClickable(true);
                setEnableModalBackgroundWrapper(true);
            }
        }
    }

    public void setEnableModalBackgroundWrapper(boolean enableModalBackgroundWrapper) {
        if (enableModalBackgroundWrapper == mEnableModalBackgroundWrapper) {
            return;
        }
        mEnableModalBackgroundWrapper = enableModalBackgroundWrapper;
        if (mAdapter != null) {
            mAdapter.notifyDataSetChanged();
        }
    }

    @Override
    public void setFastScrollAlwaysVisible(boolean alwaysShow) {
        if (alwaysShow && !mFastScrollEnabled) {
            setFastScrollEnabled(true);
        }
        if (mFastScroller != null) {
            mFastScroller.setAlwaysShow(alwaysShow);
        }
        try {
            Method method = View.class.getDeclaredMethod("computeOpaqueFlags");
            method.setAccessible(true);
            method.invoke(this);
            method = View.class.getDeclaredMethod("recomputePadding");
            method.setAccessible(true);
            method.invoke(this);
        } catch (Exception e) {
        }
        if (alwaysShow) {
            setPaddingFromScroller(true);
        }
    }

    @Override
    public void setFastScrollEnabled(boolean enabled) {
        mFastScrollEnabled = enabled;
        if (enabled) {
            if (mFastScroller == null) {
                mFastScroller = new FastScroller(getContext(), this);
            }
        } else {
            if (mFastScroller != null) {
                mFastScroller.stop();
                mFastScroller = null;
            }
        }
    }

    public void setForceHeaderListAdapter(boolean forceHeaderListAdapter) {
        mForceHeaderListAdapter = forceHeaderListAdapter;
    }

    @Override
    public void setItemChecked(int position, boolean value) {
        if (mChoiceMode == CHOICE_MODE_NONE) {
            return;
        }
        if (value && mChoiceMode == CHOICE_MODE_MULTIPLE_MODAL && mChoiceActionMode == null) {
            mChoiceActionMode = startActionMode(mMultiChoiceModeCallback);
        }
        if (mChoiceMode == CHOICE_MODE_MULTIPLE || mChoiceMode == CHOICE_MODE_MULTIPLE_MODAL) {
            boolean oldValue = mCheckStates.get(position);
            mCheckStates.put(position, value);
            if (mCheckedIdStates != null && mAdapter.hasStableIds()) {
                if (value) {
                    mCheckedIdStates.put(mAdapter.getItemId(position), position);
                } else {
                    mCheckedIdStates.delete(mAdapter.getItemId(position));
                }
            }
            if (oldValue != value) {
                if (value) {
                    mCheckedItemCount++;
                } else {
                    mCheckedItemCount--;
                }
            }
            if (mChoiceActionMode != null) {
                final long id = mAdapter.getItemId(position);
                mMultiChoiceModeCallback.onItemCheckedStateChanged(mChoiceActionMode,
                        position, id, value);
            }
        } else {
            boolean updateIds = mCheckedIdStates != null && mAdapter.hasStableIds();
            if (value || isItemChecked(position)) {
                mCheckStates.clear();
                if (updateIds) {
                    mCheckedIdStates.clear();
                }
            }
            if (value) {
                mCheckStates.put(position, true);
                if (updateIds) {
                    mCheckedIdStates.put(mAdapter.getItemId(position), position);
                }
                mCheckedItemCount = 1;
            } else if (mCheckStates.size() == 0 || !mCheckStates.valueAt(0)) {
                mCheckedItemCount = 0;
            }
        }
        updateOnScreenCheckedViews();
    }

    public void setMultiChoiceModeListener(MultiChoiceModeListener listener) {
        if (mMultiChoiceModeCallback == null) {
            mMultiChoiceModeCallback = new MultiChoiceModeWrapper();
        }
        mMultiChoiceModeCallback.setWrapped(listener);
    }

    @Override
    public void setOnItemLongClickListener(OnItemLongClickListener listener) {
        mOnItemLongClickListenerWrapper.setWrapped(listener);
    }

    @Override
    public void setOnScrollListener(OnScrollListener l) {
        super.setOnScrollListener(mOnScrollListener = l);
    }

    public void setPaddingFromScroller(boolean paddingFromScroller) {
        mPaddingFromScroller = paddingFromScroller;
        recomputePaddingFromScroller();
    }

    @Override
    public void setSelectionAfterHeaderView() {
        setSelection(mHeaderViewInfos.size());
    }

    @Override
    public void setSelector(int resID) {
        setSelector(DrawableCompat.getDrawable(getResources(), resID));
    }

    protected final void setStateOnView(View child, boolean value) {
        if (child instanceof Checkable) {
            ((Checkable) child).setChecked(value);
        } else if (USE_ACTIVATED) {
            child.setActivated(value);
        }
    }

    @Override
    public void setVerticalScrollbarPosition(int position) {
        mVerticalScrollbarPosition = position;
        if (mFastScroller != null) {
            mFastScroller.setScrollbarPosition(position);
        }
        recomputePaddingFromScroller();
    }

    @Override
    public boolean showContextMenuForChild(View originalView) {
        final int longPressPosition = getPositionForView(originalView);
        if (longPressPosition >= 0) {
            final long longPressId = mAdapter.getItemId(longPressPosition);
            boolean handled = false;
            if (mOnItemLongClickListenerWrapper.wrapped != null) {
                handled = mOnItemLongClickListenerWrapper.wrapped.onItemLongClick(ListView.this,
                        originalView, longPressPosition, longPressId);
            }
            if (!handled) {
                mContextMenuInfo = createContextMenuInfo(getChildAt(longPressPosition
                        - getFirstVisiblePosition()), longPressPosition, longPressId);
                handled = super.showContextMenuForChild(originalView);
            }
            return handled;
        }
        return false;
    }

    public ActionMode startActionMode(ActionMode.Callback callback) {
        if (mActivity != null) {
            return mActivity.startActionMode(callback);
        }
        throw new RuntimeException("HoloEverywhere.ListView (" + this
                + ") don't have reference on Activity");
    }

    private void updateOnScreenCheckedViews() {
        if (mCheckStates == null) {
            return;
        }
        final int firstPos = getFirstVisiblePosition();
        final int count = getChildCount();
        for (int i = 0; i < count; i++) {
            final View child = getChildAt(i);
            final int position = firstPos + i;
            final boolean value = mCheckStates.get(position);
            setStateOnView(child, value);
        }
    }
}
