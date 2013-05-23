
package org.holoeverywhere.widget;

import org.holoeverywhere.R;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.res.Resources;
import android.database.DataSetObserver;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.os.Build.VERSION;
import android.os.Handler;
import android.util.AttributeSet;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.MeasureSpec;
import android.view.View.OnTouchListener;
import android.view.ViewGroup;
import android.view.ViewParent;
import android.widget.AbsListView;
import android.widget.AbsListView.OnScrollListener;
import android.widget.AdapterView;
import android.widget.ListAdapter;

public class ListPopupWindow {
    private static class DropDownListView extends ListView {
        private boolean mHijackFocus;
        private boolean mListSelectionHidden;

        public DropDownListView(Context context, boolean hijackFocus) {
            super(context, null, R.attr.dropDownListViewStyle);
            mHijackFocus = hijackFocus;
        }

        @Override
        public boolean hasFocus() {
            return mHijackFocus || super.hasFocus();
        }

        @Override
        public boolean hasWindowFocus() {
            return mHijackFocus || super.hasWindowFocus();
        }

        @Override
        public boolean isFocused() {
            return mHijackFocus || super.isFocused();
        }

        @Override
        public boolean isInTouchMode() {
            return mHijackFocus && mListSelectionHidden
                    || super.isInTouchMode();
        }

        @Override
        public View onPrepareView(View view, int position) {
            if (view instanceof android.widget.TextView) {
                ((android.widget.TextView) view).setHorizontallyScrolling(true);
            }
            return view;
        }
    }

    private class ListSelectorHider implements Runnable {
        @Override
        public void run() {
            clearListSelection();
        }
    }

    private class PopupDataSetObserver extends DataSetObserver {
        @Override
        public void onChanged() {
            if (isShowing()) {
                show();
            }
        }

        @Override
        public void onInvalidated() {
            dismiss();
        }
    }

    private class PopupScrollListener implements ListView.OnScrollListener {
        @Override
        public void onScroll(AbsListView view, int firstVisibleItem,
                int visibleItemCount, int totalItemCount) {
        }

        @Override
        public void onScrollStateChanged(AbsListView view, int scrollState) {
            if (scrollState == OnScrollListener.SCROLL_STATE_TOUCH_SCROLL
                    && !isInputMethodNotNeeded()
                    && mPopup.getContentView() != null) {
                mHandler.removeCallbacks(mResizePopupRunnable);
                mResizePopupRunnable.run();
            }
        }
    }

    private class PopupTouchInterceptor implements OnTouchListener {
        @Override
        public boolean onTouch(View v, MotionEvent event) {
            final int action = event.getAction();
            final int x = (int) event.getX();
            final int y = (int) event.getY();
            if (action == MotionEvent.ACTION_DOWN && mPopup != null
                    && mPopup.isShowing() && x >= 0 && x < mPopup.getWidth()
                    && y >= 0 && y < mPopup.getHeight()) {
                mHandler.postDelayed(mResizePopupRunnable,
                        ListPopupWindow.EXPAND_LIST_TIMEOUT);
            } else if (action == MotionEvent.ACTION_UP) {
                mHandler.removeCallbacks(mResizePopupRunnable);
            }
            return false;
        }
    }

    private class ResizePopupRunnable implements Runnable {
        @Override
        public void run() {
            if (mDropDownList != null
                    && mDropDownList.getCount() > mDropDownList.getChildCount()
                    && mDropDownList.getChildCount() <= mListItemExpandMaximum) {
                mPopup.setInputMethodMode(PopupWindow.INPUT_METHOD_NOT_NEEDED);
                show();
            }
        }
    }

    private static final boolean DEBUG = false;
    private static final int EXPAND_LIST_TIMEOUT = 250;
    public static final int INPUT_METHOD_FROM_FOCUSABLE = PopupWindow.INPUT_METHOD_FROM_FOCUSABLE;
    public static final int INPUT_METHOD_NEEDED = PopupWindow.INPUT_METHOD_NEEDED;
    public static final int INPUT_METHOD_NOT_NEEDED = PopupWindow.INPUT_METHOD_NOT_NEEDED;
    public static final int MATCH_PARENT = ViewGroup.LayoutParams.MATCH_PARENT;
    public static final int POSITION_PROMPT_ABOVE = 0;
    public static final int POSITION_PROMPT_BELOW = 1;
    private static final String TAG = "ListPopupWindow";
    public static final int WRAP_CONTENT = ViewGroup.LayoutParams.WRAP_CONTENT;
    private ListAdapter mAdapter;
    private Context mContext;
    private boolean mDropDownAlwaysVisible = false;
    private View mDropDownAnchorView;
    private int mDropDownHeight = ViewGroup.LayoutParams.WRAP_CONTENT;
    private int mDropDownHorizontalOffset;
    private DropDownListView mDropDownList;
    private Drawable mDropDownListHighlight;
    private int mDropDownVerticalOffset;
    private boolean mDropDownVerticalOffsetSet;
    private int mDropDownWidth = ViewGroup.LayoutParams.WRAP_CONTENT;
    private boolean mForceIgnoreOutsideTouch = false;
    private Handler mHandler = new Handler();
    private final ListSelectorHider mHideSelector = new ListSelectorHider();
    private AdapterView.OnItemClickListener mItemClickListener;
    private AdapterView.OnItemSelectedListener mItemSelectedListener;
    int mListItemExpandMaximum = Integer.MAX_VALUE;
    private boolean mModal;
    private DataSetObserver mObserver;
    private PopupWindow mPopup;
    private int mPromptPosition = ListPopupWindow.POSITION_PROMPT_ABOVE;
    private View mPromptView;
    private final ResizePopupRunnable mResizePopupRunnable = new ResizePopupRunnable();
    private final PopupScrollListener mScrollListener = new PopupScrollListener();
    private Runnable mShowDropDownRunnable;
    private Rect mTempRect = new Rect();
    private final PopupTouchInterceptor mTouchInterceptor = new PopupTouchInterceptor();

    public ListPopupWindow(Context context) {
        this(context, null, R.attr.listPopupWindowStyle, 0);
    }

    public ListPopupWindow(Context context, AttributeSet attrs) {
        this(context, attrs, R.attr.listPopupWindowStyle, 0);
    }

    public ListPopupWindow(Context context, AttributeSet attrs, int defStyleAttr) {
        this(context, attrs, defStyleAttr, 0);
    }

    public ListPopupWindow(Context context, AttributeSet attrs, int defStyleAttr, int defStyleRes) {
        mPopup = new PopupWindow(mContext = context, attrs, defStyleAttr, defStyleRes);
        mPopup.setInputMethodMode(PopupWindow.INPUT_METHOD_NEEDED);
    }

    private int buildDropDown() {
        ViewGroup dropDownView;
        int otherHeights = 0;
        if (mDropDownList == null) {
            Context context = mContext;
            mShowDropDownRunnable = new Runnable() {
                @Override
                public void run() {
                    View view = getAnchorView();
                    if (view != null && view.getWindowToken() != null) {
                        show();
                    }
                }
            };
            mDropDownList = new DropDownListView(context, !mModal);
            if (mDropDownListHighlight != null) {
                mDropDownList.setSelector(mDropDownListHighlight);
            }
            mDropDownList.setAdapter(mAdapter);
            mDropDownList.setOnItemClickListener(mItemClickListener);
            mDropDownList.setFocusable(true);
            mDropDownList.setFocusableInTouchMode(true);
            mDropDownList
                    .setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
                        @Override
                        public void onItemSelected(AdapterView<?> parent,
                                View view, int position, long id) {

                            if (position != -1) {
                                DropDownListView dropDownList = mDropDownList;

                                if (dropDownList != null) {
                                    dropDownList.mListSelectionHidden = false;
                                }
                            }
                        }

                        @Override
                        public void onNothingSelected(AdapterView<?> parent) {
                        }
                    });
            mDropDownList.setOnScrollListener(mScrollListener);
            if (mItemSelectedListener != null) {
                mDropDownList.setOnItemSelectedListener(mItemSelectedListener);
            }
            dropDownView = mDropDownList;
            View hintView = mPromptView;
            if (hintView != null) {
                LinearLayout hintContainer = new LinearLayout(context);
                hintContainer
                        .setOrientation(android.widget.LinearLayout.VERTICAL);
                LinearLayout.LayoutParams hintParams = new LinearLayout.LayoutParams(
                        ViewGroup.LayoutParams.MATCH_PARENT, 0, 1.0f);
                switch (mPromptPosition) {
                    case POSITION_PROMPT_BELOW:
                        hintContainer.addView(dropDownView, hintParams);
                        hintContainer.addView(hintView);
                        break;
                    case POSITION_PROMPT_ABOVE:
                        hintContainer.addView(hintView);
                        hintContainer.addView(dropDownView, hintParams);
                        break;
                    default:
                        Log.e(ListPopupWindow.TAG, "Invalid hint position "
                                + mPromptPosition);
                        break;
                }
                int widthSpec = MeasureSpec.makeMeasureSpec(mDropDownWidth,
                        MeasureSpec.AT_MOST);
                int heightSpec = MeasureSpec.UNSPECIFIED;
                hintView.measure(widthSpec, heightSpec);
                hintParams = (LinearLayout.LayoutParams) hintView
                        .getLayoutParams();
                otherHeights = hintView.getMeasuredHeight()
                        + hintParams.topMargin + hintParams.bottomMargin;
                dropDownView = hintContainer;
            }
            mPopup.setContentView(dropDownView);
        } else {
            dropDownView = (ViewGroup) mPopup.getContentView();
            final View view = mPromptView;
            if (view != null) {
                LinearLayout.LayoutParams hintParams = (LinearLayout.LayoutParams) view
                        .getLayoutParams();
                otherHeights = view.getMeasuredHeight() + hintParams.topMargin
                        + hintParams.bottomMargin;
            }
        }
        int padding = 0;
        Drawable background = mPopup.getBackground();
        if (background != null) {
            background.getPadding(mTempRect);
            padding = mTempRect.top + mTempRect.bottom;
            if (!mDropDownVerticalOffsetSet) {
                mDropDownVerticalOffset = -mTempRect.top;
            }
        } else {
            mTempRect.setEmpty();
        }
        boolean ignoreBottomDecorations = mPopup.getInputMethodMode() == PopupWindow.INPUT_METHOD_NOT_NEEDED;
        final int maxHeight = getMaxAvailableHeight(getAnchorView(),
                mDropDownVerticalOffset, ignoreBottomDecorations);
        if (mDropDownAlwaysVisible
                || mDropDownHeight == ViewGroup.LayoutParams.MATCH_PARENT) {
            return maxHeight + padding;
        }
        final int childWidthSpec;
        switch (mDropDownWidth) {
            case ViewGroup.LayoutParams.WRAP_CONTENT:
                childWidthSpec = MeasureSpec.makeMeasureSpec(mContext
                        .getResources().getDisplayMetrics().widthPixels
                        - (mTempRect.left + mTempRect.right), MeasureSpec.AT_MOST);
                break;
            case ViewGroup.LayoutParams.MATCH_PARENT:
                childWidthSpec = MeasureSpec.makeMeasureSpec(mContext
                        .getResources().getDisplayMetrics().widthPixels
                        - (mTempRect.left + mTempRect.right), MeasureSpec.EXACTLY);
                break;
            default:
                childWidthSpec = MeasureSpec.makeMeasureSpec(mDropDownWidth,
                        MeasureSpec.EXACTLY);
                break;
        }
        final int listContent = measureHeightOfChildren(childWidthSpec, 0, -1,
                maxHeight - otherHeights, -1);
        if (listContent > 0) {
            otherHeights += padding;
        }
        return listContent + otherHeights;
    }

    public void clearListSelection() {
        final DropDownListView list = mDropDownList;
        if (list != null) {
            list.mListSelectionHidden = true;
            // TODO list.hideSelector();
            list.requestLayout();
        }
    }

    public void dismiss() {
        mPopup.dismiss();
        removePromptView();
        mPopup.setContentView(null);
        mDropDownList = null;
        mHandler.removeCallbacks(mResizePopupRunnable);
    }

    public View getAnchorView() {
        return mDropDownAnchorView;
    }

    public int getAnimationStyle() {
        return mPopup.getAnimationStyle();
    }

    public Drawable getBackground() {
        return mPopup.getBackground();
    }

    public int getHeight() {
        return mDropDownHeight;
    }

    public int getHorizontalOffset() {
        return mDropDownHorizontalOffset;
    }

    public int getInputMethodMode() {
        return mPopup.getInputMethodMode();
    }

    public ListView getListView() {
        return mDropDownList;
    }

    private int getMaxAvailableHeight(View anchor, int yOffset,
            boolean ignoreBottomDecorations) {
        final Rect displayFrame = new Rect();
        anchor.getWindowVisibleDisplayFrame(displayFrame);
        final int[] anchorPos = new int[2];
        anchor.getLocationOnScreen(anchorPos);
        int bottomEdge = displayFrame.bottom;
        if (ignoreBottomDecorations) {
            Resources res = anchor.getContext().getResources();
            bottomEdge = res.getDisplayMetrics().heightPixels;
        }
        final int distanceToBottom = bottomEdge
                - (anchorPos[1] + anchor.getHeight()) - yOffset;
        final int distanceToTop = anchorPos[1] - displayFrame.top + yOffset;
        int returnedHeight = Math.max(distanceToBottom, distanceToTop);
        if (mPopup.getBackground() != null) {
            mPopup.getBackground().getPadding(mTempRect);
            returnedHeight -= mTempRect.top + mTempRect.bottom;
        }
        return returnedHeight;
    }

    public int getPromptPosition() {
        return mPromptPosition;
    }

    public Object getSelectedItem() {
        if (!isShowing()) {
            return null;
        }
        return mDropDownList.getSelectedItem();
    }

    public long getSelectedItemId() {
        if (!isShowing()) {
            return AdapterView.INVALID_ROW_ID;
        }
        return mDropDownList.getSelectedItemId();
    }

    public int getSelectedItemPosition() {
        if (!isShowing()) {
            return AdapterView.INVALID_POSITION;
        }
        return mDropDownList.getSelectedItemPosition();
    }

    public View getSelectedView() {
        if (!isShowing()) {
            return null;
        }
        return mDropDownList.getSelectedView();
    }

    public int getSoftInputMode() {
        return mPopup.getSoftInputMode();
    }

    public int getVerticalOffset() {
        if (!mDropDownVerticalOffsetSet) {
            return 0;
        }
        return mDropDownVerticalOffset;
    }

    public int getWidth() {
        return mDropDownWidth;
    }

    public boolean isDropDownAlwaysVisible() {
        return mDropDownAlwaysVisible;
    }

    public boolean isInputMethodNotNeeded() {
        return mPopup.getInputMethodMode() == ListPopupWindow.INPUT_METHOD_NOT_NEEDED;
    }

    public boolean isModal() {
        return mModal;
    }

    public boolean isShowing() {
        return mPopup.isShowing();
    }

    private int measureHeightOfChildren(int widthMeasureSpec,
            int startPosition, int endPosition, final int maxHeight,
            int disallowPartialChildPosition) {
        final ListAdapter adapter = mAdapter;
        if (adapter == null) {
            return mDropDownList.getListPaddingTop()
                    + mDropDownList.getListPaddingBottom();
        }
        int returnedHeight = mDropDownList.getListPaddingTop()
                + mDropDownList.getListPaddingBottom();
        final int dividerHeight = mDropDownList.getDividerHeight() > 0
                && mDropDownList.getDivider() != null ? mDropDownList
                .getDividerHeight() : 0;
        int prevHeightWithoutPartialChild = 0;
        int i;
        View child;
        endPosition = endPosition == -1 ? adapter.getCount() - 1 : endPosition;
        for (i = startPosition; i <= endPosition; ++i) {
            child = mAdapter.getView(i, null, mDropDownList);
            if (mDropDownList.getCacheColorHint() != 0) {
                child.setDrawingCacheBackgroundColor(mDropDownList
                        .getCacheColorHint());
            }
            measureScrapChild(child, i, widthMeasureSpec);
            if (i > 0) {
                returnedHeight += dividerHeight;
            }
            returnedHeight += child.getMeasuredHeight();
            if (returnedHeight >= maxHeight) {
                return disallowPartialChildPosition >= 0
                        && i > disallowPartialChildPosition
                        && prevHeightWithoutPartialChild > 0
                        && returnedHeight != maxHeight ? prevHeightWithoutPartialChild
                        : maxHeight;
            }
            if (disallowPartialChildPosition >= 0
                    && i >= disallowPartialChildPosition) {
                prevHeightWithoutPartialChild = returnedHeight;
            }
        }
        return returnedHeight;
    }

    private void measureScrapChild(View child, int position,
            int widthMeasureSpec) {
        ListView.LayoutParams p = (ListView.LayoutParams) child
                .getLayoutParams();
        if (p == null) {
            p = new ListView.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT, 0);
            child.setLayoutParams(p);
        }
        // TODO p.viewType = mAdapter.getItemViewType(position);
        // TODO p.forceAdd = true;
        int childWidthSpec = ViewGroup.getChildMeasureSpec(
                widthMeasureSpec,
                mDropDownList.getPaddingLeft()
                        + mDropDownList.getPaddingRight(), p.width);
        int lpHeight = p.height;
        int childHeightSpec;
        if (lpHeight > 0) {
            childHeightSpec = MeasureSpec.makeMeasureSpec(lpHeight,
                    MeasureSpec.EXACTLY);
        } else {
            childHeightSpec = MeasureSpec.makeMeasureSpec(0,
                    MeasureSpec.UNSPECIFIED);
        }
        child.measure(childWidthSpec, childHeightSpec);
    }

    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (isShowing()) {
            if (keyCode != KeyEvent.KEYCODE_SPACE
                    && (mDropDownList.getSelectedItemPosition() >= 0 || keyCode != KeyEvent.KEYCODE_ENTER
                            && keyCode != KeyEvent.KEYCODE_DPAD_CENTER)) {
                int curIndex = mDropDownList.getSelectedItemPosition();
                boolean consumed;
                final boolean below = !mPopup.isAboveAnchor();
                final ListAdapter adapter = mAdapter;
                @SuppressWarnings("unused")
                boolean allEnabled;
                int firstItem = Integer.MAX_VALUE;
                int lastItem = Integer.MIN_VALUE;
                if (adapter != null) {
                    allEnabled = adapter.areAllItemsEnabled();
                    firstItem = 0;
                    lastItem = adapter.getCount() - 1;
                    /*
                     * TODO firstItem = allEnabled ? 0 : mDropDownList
                     * .lookForSelectablePosition(0, true); lastItem =
                     * allEnabled ? adapter.getCount() - 1 :
                     * mDropDownList.lookForSelectablePosition(
                     * adapter.getCount() - 1, false);
                     */
                }
                if (below && keyCode == KeyEvent.KEYCODE_DPAD_UP
                        && curIndex <= firstItem || !below
                        && keyCode == KeyEvent.KEYCODE_DPAD_DOWN
                        && curIndex >= lastItem) {
                    clearListSelection();
                    mPopup.setInputMethodMode(PopupWindow.INPUT_METHOD_NEEDED);
                    show();
                    return true;
                } else {
                    mDropDownList.mListSelectionHidden = false;
                }
                consumed = mDropDownList.onKeyDown(keyCode, event);
                if (ListPopupWindow.DEBUG) {
                    Log.v(ListPopupWindow.TAG, "Key down: code=" + keyCode
                            + " list consumed=" + consumed);
                }

                if (consumed) {
                    mPopup.setInputMethodMode(PopupWindow.INPUT_METHOD_NOT_NEEDED);
                    mDropDownList.requestFocusFromTouch();
                    show();
                    switch (keyCode) {
                        case KeyEvent.KEYCODE_ENTER:
                        case KeyEvent.KEYCODE_DPAD_CENTER:
                        case KeyEvent.KEYCODE_DPAD_DOWN:
                        case KeyEvent.KEYCODE_DPAD_UP:
                            return true;
                    }
                } else {
                    if (below && keyCode == KeyEvent.KEYCODE_DPAD_DOWN) {
                        if (curIndex == lastItem) {
                            return true;
                        }
                    } else if (!below && keyCode == KeyEvent.KEYCODE_DPAD_UP
                            && curIndex == firstItem) {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    @SuppressLint("NewApi")
    public boolean onKeyPreIme(int keyCode, KeyEvent event) {
        if (VERSION.SDK_INT < 5) {
            return false;
        }
        if (keyCode == KeyEvent.KEYCODE_BACK && isShowing()) {
            final View anchorView = mDropDownAnchorView;
            if (event.getAction() == KeyEvent.ACTION_DOWN
                    && event.getRepeatCount() == 0) {
                KeyEvent.DispatcherState state = anchorView
                        .getKeyDispatcherState();
                if (state != null) {
                    state.startTracking(event, this);
                }
                return true;
            } else if (event.getAction() == KeyEvent.ACTION_UP) {
                KeyEvent.DispatcherState state = anchorView
                        .getKeyDispatcherState();
                if (state != null) {
                    state.handleUpEvent(event);
                }
                if (event.isTracking() && !event.isCanceled()) {
                    dismiss();
                    return true;
                }
            }
        }
        return false;
    }

    public boolean onKeyUp(int keyCode, KeyEvent event) {
        if (isShowing() && mDropDownList.getSelectedItemPosition() >= 0) {
            boolean consumed = mDropDownList.onKeyUp(keyCode, event);
            if (consumed) {
                switch (keyCode) {
                    case KeyEvent.KEYCODE_ENTER:
                    case KeyEvent.KEYCODE_DPAD_CENTER:
                        dismiss();
                        break;
                }
            }
            return consumed;
        }
        return false;
    }

    public boolean performItemClick(int position) {
        if (isShowing()) {
            if (mItemClickListener != null) {
                final DropDownListView list = mDropDownList;
                final View child = list.getChildAt(position
                        - list.getFirstVisiblePosition());
                final ListAdapter adapter = list.getAdapter();
                mItemClickListener.onItemClick(list, child, position,
                        adapter.getItemId(position));
            }
            return true;
        }
        return false;
    }

    public void postShow() {
        mHandler.post(mShowDropDownRunnable);
    }

    private void removePromptView() {
        if (mPromptView != null) {
            final ViewParent parent = mPromptView.getParent();
            if (parent instanceof ViewGroup) {
                final ViewGroup group = (ViewGroup) parent;
                group.removeView(mPromptView);
            }
        }
    }

    public void setAdapter(ListAdapter adapter) {
        if (mObserver == null) {
            mObserver = new PopupDataSetObserver();
        } else if (mAdapter != null) {
            mAdapter.unregisterDataSetObserver(mObserver);
        }
        mAdapter = adapter;
        if (mAdapter != null) {
            mAdapter.registerDataSetObserver(mObserver);
        }
        if (mDropDownList != null) {
            mDropDownList.setAdapter(mAdapter);
        }
    }

    public void setAnchorView(View anchor) {
        mDropDownAnchorView = anchor;
    }

    public void setAnimationStyle(int animationStyle) {
        mPopup.setAnimationStyle(animationStyle);
    }

    public void setBackgroundDrawable(Drawable d) {
        mPopup.setBackgroundDrawable(d);
    }

    public void setContentWidth(int width) {
        Drawable popupBackground = mPopup.getBackground();
        if (popupBackground != null) {
            popupBackground.getPadding(mTempRect);
            mDropDownWidth = mTempRect.left + mTempRect.right + width;
        } else {
            setWidth(width);
        }
    }

    public void setDropDownAlwaysVisible(boolean dropDownAlwaysVisible) {
        mDropDownAlwaysVisible = dropDownAlwaysVisible;
    }

    public void setForceIgnoreOutsideTouch(boolean forceIgnoreOutsideTouch) {
        mForceIgnoreOutsideTouch = forceIgnoreOutsideTouch;
    }

    public void setHeight(int height) {
        mDropDownHeight = height;
    }

    public void setHorizontalOffset(int offset) {
        mDropDownHorizontalOffset = offset;
    }

    public void setInputMethodMode(int mode) {
        mPopup.setInputMethodMode(mode);
    }

    void setListItemExpandMax(int max) {
        mListItemExpandMaximum = max;
    }

    public void setListSelector(Drawable selector) {
        mDropDownListHighlight = selector;
    }

    public void setModal(boolean modal) {
        mModal = true;
        mPopup.setFocusable(modal);
    }

    public void setOnDismissListener(PopupWindow.OnDismissListener listener) {
        mPopup.setOnDismissListener(listener);
    }

    public void setOnItemClickListener(
            AdapterView.OnItemClickListener clickListener) {
        mItemClickListener = clickListener;
    }

    public void setOnItemSelectedListener(
            AdapterView.OnItemSelectedListener selectedListener) {
        mItemSelectedListener = selectedListener;
    }

    public void setPromptPosition(int position) {
        mPromptPosition = position;
    }

    public void setPromptView(View prompt) {
        boolean showing = isShowing();
        if (showing) {
            removePromptView();
        }
        mPromptView = prompt;
        if (showing) {
            show();
        }
    }

    public void setSelection(int position) {
        DropDownListView list = mDropDownList;
        if (isShowing() && list != null) {
            list.mListSelectionHidden = false;
            list.setSelection(position);
            if (list.getChoiceMode() != AbsListView.CHOICE_MODE_NONE) {
                list.setItemChecked(position, true);
            }
        }
    }

    public void setSoftInputMode(int mode) {
        mPopup.setSoftInputMode(mode);
    }

    public void setVerticalOffset(int offset) {
        mDropDownVerticalOffset = offset;
        mDropDownVerticalOffsetSet = true;
    }

    public void setWidth(int width) {
        mDropDownWidth = width;
    }

    public void show() {
        int height = buildDropDown();
        int widthSpec = 0;
        int heightSpec = 0;
        boolean noInputMethod = isInputMethodNotNeeded();
        mPopup.setAllowScrollingAnchorParent(!noInputMethod);
        if (mPopup.isShowing()) {
            if (mDropDownWidth == ViewGroup.LayoutParams.MATCH_PARENT) {
                widthSpec = -1;
            } else if (mDropDownWidth == ViewGroup.LayoutParams.WRAP_CONTENT) {
                widthSpec = getAnchorView().getWidth();
            } else {
                widthSpec = mDropDownWidth;
            }
            if (mDropDownHeight == ViewGroup.LayoutParams.MATCH_PARENT) {
                heightSpec = noInputMethod ? height
                        : ViewGroup.LayoutParams.MATCH_PARENT;
                if (noInputMethod) {
                    mPopup.setWindowLayoutMode(
                            mDropDownWidth == ViewGroup.LayoutParams.MATCH_PARENT ? ViewGroup.LayoutParams.MATCH_PARENT
                                    : 0, 0);
                } else {
                    mPopup.setWindowLayoutMode(
                            mDropDownWidth == ViewGroup.LayoutParams.MATCH_PARENT ? ViewGroup.LayoutParams.MATCH_PARENT
                                    : 0, ViewGroup.LayoutParams.MATCH_PARENT);
                }
            } else if (mDropDownHeight == ViewGroup.LayoutParams.WRAP_CONTENT) {
                heightSpec = height;
            } else {
                heightSpec = mDropDownHeight;
            }
            mPopup.setOutsideTouchable(!mForceIgnoreOutsideTouch
                    && !mDropDownAlwaysVisible);
            mPopup.update(getAnchorView(), mDropDownHorizontalOffset,
                    mDropDownVerticalOffset, widthSpec, heightSpec);
        } else {
            if (mDropDownWidth == ViewGroup.LayoutParams.MATCH_PARENT) {
                widthSpec = ViewGroup.LayoutParams.MATCH_PARENT;
            } else {
                if (mDropDownWidth == ViewGroup.LayoutParams.WRAP_CONTENT) {
                    mPopup.setWidth(getAnchorView().getWidth());
                } else {
                    mPopup.setWidth(mDropDownWidth);
                }
            }
            if (mDropDownHeight == ViewGroup.LayoutParams.MATCH_PARENT) {
                heightSpec = ViewGroup.LayoutParams.MATCH_PARENT;
            } else {
                if (mDropDownHeight == ViewGroup.LayoutParams.WRAP_CONTENT) {
                    mPopup.setHeight(height);
                } else {
                    mPopup.setHeight(mDropDownHeight);
                }
            }
            mPopup.setWindowLayoutMode(widthSpec, heightSpec);
            mPopup.setClipToScreenEnabled(true);
            mPopup.setOutsideTouchable(!mForceIgnoreOutsideTouch
                    && !mDropDownAlwaysVisible);
            mPopup.setTouchInterceptor(mTouchInterceptor);
            mPopup.showAsDropDown(getAnchorView(), mDropDownHorizontalOffset,
                    mDropDownVerticalOffset);
            mDropDownList.setSelection(AdapterView.INVALID_POSITION);
            if (!mModal || mDropDownList.isInTouchMode()) {
                clearListSelection();
            }
            if (!mModal) {
                mHandler.post(mHideSelector);
            }
        }
    }
}
