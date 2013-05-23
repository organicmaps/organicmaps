
package org.holoeverywhere.widget;

import java.util.ArrayList;

import org.holoeverywhere.R;
import org.holoeverywhere.widget.ExpandableListConnector.PositionMetadata;

import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.os.Build.VERSION;
import android.os.Build.VERSION_CODES;
import android.os.Parcel;
import android.os.Parcelable;
import android.util.AttributeSet;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.SoundEffectConstants;
import android.view.View;
import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityNodeInfo;
import android.widget.ExpandableListAdapter;
import android.widget.ListAdapter;

public class ExpandableListView extends ListView {
    public static class ExpandableListContextMenuInfo implements ContextMenu.ContextMenuInfo {
        public long id;
        public long packedPosition;
        public View targetView;

        public ExpandableListContextMenuInfo(View targetView, long packedPosition, long id) {
            this.targetView = targetView;
            this.packedPosition = packedPosition;
            this.id = id;
        }
    }

    public interface OnChildClickListener {
        boolean onChildClick(ExpandableListView parent, View v, int groupPosition,
                int childPosition, long id);
    }

    public interface OnGroupClickListener {
        boolean onGroupClick(ExpandableListView parent, View v, int groupPosition,
                long id);
    }

    public interface OnGroupCollapseListener {
        void onGroupCollapse(int groupPosition);
    }

    public interface OnGroupExpandListener {
        void onGroupExpand(int groupPosition);
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
        ArrayList<ExpandableListConnector.GroupMetadata> expandedGroupMetadataList;

        private SavedState(Parcel in) {
            super(in.readParcelable(ListView.SavedState.class.getClassLoader()));
            expandedGroupMetadataList = new ArrayList<ExpandableListConnector.GroupMetadata>();
            in.readList(expandedGroupMetadataList, ExpandableListConnector.class.getClassLoader());
        }

        SavedState(
                Parcelable superState,
                ArrayList<ExpandableListConnector.GroupMetadata> expandedGroupMetadataList) {
            super(superState);
            this.expandedGroupMetadataList = expandedGroupMetadataList;
        }

        @Override
        public void writeToParcel(Parcel out, int flags) {
            super.writeToParcel(out, flags);
            out.writeList(expandedGroupMetadataList);
        }
    }

    public static final int CHILD_INDICATOR_INHERIT = -1;
    private static final int[] CHILD_LAST_STATE_SET =
    {
            android.R.attr.state_last
    };

    private static final int[] EMPTY_STATE_SET = {};
    private static final int[] GROUP_EMPTY_STATE_SET =
    {
            android.R.attr.state_empty
    };
    private static final int[] GROUP_EXPANDED_EMPTY_STATE_SET =
    {
            android.R.attr.state_expanded, android.R.attr.state_empty
    };
    private static final int[] GROUP_EXPANDED_STATE_SET =
    {
            android.R.attr.state_expanded
    };
    private static final int[][] GROUP_STATE_SETS = {
            EMPTY_STATE_SET,
            GROUP_EXPANDED_STATE_SET,
            GROUP_EMPTY_STATE_SET,
            GROUP_EXPANDED_EMPTY_STATE_SET
    };

    private static final long PACKED_POSITION_INT_MASK_CHILD = 0xFFFFFFFF;
    private static final long PACKED_POSITION_INT_MASK_GROUP = 0x7FFFFFFF;
    private static final long PACKED_POSITION_MASK_CHILD = 0x00000000FFFFFFFFL;
    private static final long PACKED_POSITION_MASK_GROUP = 0x7FFFFFFF00000000L;
    private static final long PACKED_POSITION_MASK_TYPE = 0x8000000000000000L;
    private static final long PACKED_POSITION_SHIFT_GROUP = 32;
    private static final long PACKED_POSITION_SHIFT_TYPE = 63;
    public static final int PACKED_POSITION_TYPE_CHILD = 1;
    public static final int PACKED_POSITION_TYPE_GROUP = 0;
    public static final int PACKED_POSITION_TYPE_NULL = 2;
    public static final long PACKED_POSITION_VALUE_NULL = 0x00000000FFFFFFFFL;

    public static int getPackedPositionChild(long packedPosition) {
        if (packedPosition == PACKED_POSITION_VALUE_NULL) {
            return -1;
        }
        if ((packedPosition & PACKED_POSITION_MASK_TYPE) != PACKED_POSITION_MASK_TYPE) {
            return -1;
        }
        return (int) (packedPosition & PACKED_POSITION_MASK_CHILD);
    }

    public static long getPackedPositionForChild(int groupPosition, int childPosition) {
        return (long) PACKED_POSITION_TYPE_CHILD << PACKED_POSITION_SHIFT_TYPE
                | (groupPosition & PACKED_POSITION_INT_MASK_GROUP)
                << PACKED_POSITION_SHIFT_GROUP
                | childPosition & PACKED_POSITION_INT_MASK_CHILD;
    }

    public static long getPackedPositionForGroup(int groupPosition) {
        return (groupPosition & PACKED_POSITION_INT_MASK_GROUP)
        << PACKED_POSITION_SHIFT_GROUP;
    }

    public static int getPackedPositionGroup(long packedPosition) {
        if (packedPosition == PACKED_POSITION_VALUE_NULL) {
            return -1;
        }
        return (int) ((packedPosition & PACKED_POSITION_MASK_GROUP) >> PACKED_POSITION_SHIFT_GROUP);
    }

    public static int getPackedPositionType(long packedPosition) {
        if (packedPosition == PACKED_POSITION_VALUE_NULL) {
            return PACKED_POSITION_TYPE_NULL;
        }
        return (packedPosition & PACKED_POSITION_MASK_TYPE) == PACKED_POSITION_MASK_TYPE
                ? PACKED_POSITION_TYPE_CHILD
                : PACKED_POSITION_TYPE_GROUP;
    }

    private ExpandableListAdapter mAdapter;
    private Drawable mChildDivider;
    private Drawable mChildIndicator;
    private int mChildIndicatorLeft;
    private int mChildIndicatorRight;
    private boolean mClipToPadding = false;
    private ExpandableListConnector mConnector;
    private Drawable mGroupIndicator;
    private int mIndicatorLeft;
    private final Rect mIndicatorRect = new Rect();
    private int mIndicatorRight;
    private OnChildClickListener mOnChildClickListener;
    private OnGroupClickListener mOnGroupClickListener;
    private OnGroupCollapseListener mOnGroupCollapseListener;
    private OnGroupExpandListener mOnGroupExpandListener;

    public ExpandableListView(Context context) {
        this(context, null);
    }

    public ExpandableListView(Context context, AttributeSet attrs) {
        this(context, attrs, android.R.attr.expandableListViewStyle);
    }

    public ExpandableListView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        TypedArray a = context.obtainStyledAttributes(attrs, R.styleable.ExpandableListView,
                defStyle, R.style.Holo_ExpandableListView);
        mGroupIndicator = a
                .getDrawable(R.styleable.ExpandableListView_android_groupIndicator);
        mChildIndicator = a
                .getDrawable(R.styleable.ExpandableListView_android_childIndicator);
        mIndicatorLeft = a
                .getDimensionPixelSize(
                        R.styleable.ExpandableListView_android_indicatorLeft, 0);
        mIndicatorRight = a
                .getDimensionPixelSize(
                        R.styleable.ExpandableListView_android_indicatorRight, 0);
        if (mIndicatorRight == 0 && mGroupIndicator != null) {
            mIndicatorRight = mIndicatorLeft + mGroupIndicator.getIntrinsicWidth();
        }
        mChildIndicatorLeft = a.getDimensionPixelSize(
                R.styleable.ExpandableListView_android_childIndicatorLeft,
                CHILD_INDICATOR_INHERIT);
        mChildIndicatorRight = a.getDimensionPixelSize(
                R.styleable.ExpandableListView_android_childIndicatorRight,
                CHILD_INDICATOR_INHERIT);
        mChildDivider = a
                .getDrawable(R.styleable.ExpandableListView_android_childDivider);

        a.recycle();
    }

    public boolean collapseGroup(int groupPos) {
        boolean retValue = mConnector.collapseGroup(groupPos);
        if (mOnGroupCollapseListener != null) {
            mOnGroupCollapseListener.onGroupCollapse(groupPos);
        }
        return retValue;
    }

    @Override
    protected ContextMenuInfo createContextMenuInfo(View view, int flatListPosition, long id) {
        if (isHeaderOrFooterPosition(flatListPosition)) {
            return super.createContextMenuInfo(view, flatListPosition, id);
        }
        final int adjustedPosition = getFlatPositionForConnector(flatListPosition);
        PositionMetadata pm = mConnector.getUnflattenedPos(adjustedPosition);
        ExpandableListPosition pos = pm.position;
        id = getChildOrGroupId(pos);
        long packedPosition = pos.getPackedPosition();
        pm.recycle();
        return new ExpandableListContextMenuInfo(view, packedPosition, id);
    }

    @Override
    protected void dispatchDraw(Canvas canvas) {
        super.dispatchDraw(canvas);
        if (mChildIndicator == null && mGroupIndicator == null) {
            return;
        }
        int saveCount = 0;
        final boolean clipToPadding = mClipToPadding;
        if (clipToPadding) {
            saveCount = canvas.save();
            final int scrollX = getScrollX();
            final int scrollY = getScrollY();
            canvas.clipRect(scrollX + getPaddingLeft(), scrollY + getPaddingTop(),
                    scrollX + getRight() - getLeft() - getPaddingRight(),
                    scrollY + getBottom() - getTop() - getPaddingBottom());
        }
        final int headerViewsCount = getHeaderViewsCount();
        final int lastChildFlPos = getCount() - getFooterViewsCount() - headerViewsCount - 1;
        final int myB = getBottom();
        PositionMetadata pos;
        View item;
        Drawable indicator;
        int t, b;
        int lastItemType = ~(ExpandableListPosition.CHILD | ExpandableListPosition.GROUP);
        final Rect indicatorRect = mIndicatorRect;
        final int childCount = getChildCount();
        for (int i = 0, childFlPos = getFirstVisiblePosition() - headerViewsCount; i < childCount; i++, childFlPos++) {
            if (childFlPos < 0) {
                continue;
            } else if (childFlPos > lastChildFlPos) {
                break;
            }
            item = getChildAt(i);
            t = item.getTop();
            b = item.getBottom();
            if (b < 0 || t > myB) {
                continue;
            }
            pos = mConnector.getUnflattenedPos(childFlPos);
            if (pos.position.type != lastItemType) {
                if (pos.position.type == ExpandableListPosition.CHILD) {
                    indicatorRect.left = mChildIndicatorLeft == CHILD_INDICATOR_INHERIT ?
                            mIndicatorLeft : mChildIndicatorLeft;
                    indicatorRect.right = mChildIndicatorRight == CHILD_INDICATOR_INHERIT ?
                            mIndicatorRight : mChildIndicatorRight;
                } else {
                    indicatorRect.left = mIndicatorLeft;
                    indicatorRect.right = mIndicatorRight;
                }
                indicatorRect.left += getPaddingLeft();
                indicatorRect.right += getPaddingLeft();
                lastItemType = pos.position.type;
            }

            if (indicatorRect.left != indicatorRect.right) {
                if (isStackFromBottom()) {
                    indicatorRect.top = t;
                    indicatorRect.bottom = b;
                } else {
                    indicatorRect.top = t;
                    indicatorRect.bottom = b;
                }
                indicator = getIndicator(pos);
                if (indicator != null) {
                    indicator.setBounds(indicatorRect);
                    indicator.draw(canvas);
                }
            }
            pos.recycle();
        }

        if (clipToPadding) {
            canvas.restoreToCount(saveCount);
        }
    }

    @Override
    void drawDivider(Canvas canvas, Rect bounds, int childIndex) {
        int flatListPosition = childIndex + getFirstVisiblePosition();
        if (flatListPosition >= 0) {
            final int adjustedPosition =
                    getFlatPositionForConnector(flatListPosition);
            PositionMetadata pos = mConnector.getUnflattenedPos(adjustedPosition);
            if (pos.position.type ==
                    ExpandableListPosition.CHILD || pos.isExpanded() &&
                    pos.groupMetadata.lastChildFlPos != pos.groupMetadata.flPos) {
                Drawable divider = mChildDivider;
                divider.setBounds(bounds);
                divider.draw(canvas);
                pos.recycle();
                return;
            }
            pos.recycle();
        }
        super.drawDivider(canvas, bounds, flatListPosition);
    }

    public boolean expandGroup(int groupPos) {
        return expandGroup(groupPos, false);
    }

    public boolean expandGroup(int groupPos, boolean animate) {
        ExpandableListPosition elGroupPos = ExpandableListPosition.obtain(
                ExpandableListPosition.GROUP, groupPos, -1, -1);
        PositionMetadata pm = mConnector.getFlattenedPos(elGroupPos);
        elGroupPos.recycle();
        boolean retValue = mConnector.expandGroup(pm);
        if (mOnGroupExpandListener != null) {
            mOnGroupExpandListener.onGroupExpand(groupPos);
        }
        // TODO Make it works on Eclair
        if (animate && VERSION.SDK_INT >= VERSION_CODES.FROYO) {
            final int groupFlatPos = pm.position.flatListPos;
            final int shiftedGroupPosition = groupFlatPos + getHeaderViewsCount();
            smoothScrollToPosition(shiftedGroupPosition + mAdapter.getChildrenCount(groupPos),
                    shiftedGroupPosition);
        }
        pm.recycle();
        return retValue;
    }

    private int getAbsoluteFlatPosition(int flatListPosition) {
        return flatListPosition + getHeaderViewsCount();
    }

    private long getChildOrGroupId(ExpandableListPosition position) {
        if (position.type == ExpandableListPosition.CHILD) {
            return mAdapter.getChildId(position.groupPos, position.childPos);
        } else {
            return mAdapter.getGroupId(position.groupPos);
        }
    }

    public ExpandableListAdapter getExpandableListAdapter() {
        return mAdapter;
    }

    public long getExpandableListPosition(int flatListPosition) {
        if (isHeaderOrFooterPosition(flatListPosition)) {
            return PACKED_POSITION_VALUE_NULL;
        }
        final int adjustedPosition = getFlatPositionForConnector(flatListPosition);
        PositionMetadata pm = mConnector.getUnflattenedPos(adjustedPosition);
        long packedPos = pm.position.getPackedPosition();
        pm.recycle();
        return packedPos;
    }

    public int getFlatListPosition(long packedPosition) {
        ExpandableListPosition elPackedPos = ExpandableListPosition
                .obtainPosition(packedPosition);
        PositionMetadata pm = mConnector.getFlattenedPos(elPackedPos);
        elPackedPos.recycle();
        final int flatListPosition = pm.position.flatListPos;
        pm.recycle();
        return getAbsoluteFlatPosition(flatListPosition);
    }

    private int getFlatPositionForConnector(int flatListPosition) {
        return flatListPosition - getHeaderViewsCount();
    }

    private Drawable getIndicator(PositionMetadata pos) {
        Drawable indicator;
        if (pos.position.type == ExpandableListPosition.GROUP) {
            indicator = mGroupIndicator;
            if (indicator != null && indicator.isStateful()) {
                boolean isEmpty = pos.groupMetadata == null ||
                        pos.groupMetadata.lastChildFlPos == pos.groupMetadata.flPos;
                final int stateSetIndex =
                        (pos.isExpanded() ? 1 : 0) | // Expanded?
                                (isEmpty ? 2 : 0); // Empty?
                indicator.setState(GROUP_STATE_SETS[stateSetIndex]);
            }
        } else {
            indicator = mChildIndicator;
            if (indicator != null && indicator.isStateful()) {
                final int stateSet[] = pos.position.flatListPos == pos.groupMetadata.lastChildFlPos
                        ? CHILD_LAST_STATE_SET
                        : EMPTY_STATE_SET;
                indicator.setState(stateSet);
            }
        }
        return indicator;
    }

    public long getSelectedId() {
        long packedPos = getSelectedPosition();
        if (packedPos == PACKED_POSITION_VALUE_NULL) {
            return -1;
        }
        int groupPos = getPackedPositionGroup(packedPos);
        if (getPackedPositionType(packedPos) == PACKED_POSITION_TYPE_GROUP) {
            return mAdapter.getGroupId(groupPos);
        } else {
            return mAdapter.getChildId(groupPos, getPackedPositionChild(packedPos));
        }
    }

    public long getSelectedPosition() {
        final int selectedPos = getSelectedItemPosition();
        return getExpandableListPosition(selectedPos);
    }

    boolean handleItemClick(View v, int position, long id) {
        final PositionMetadata posMetadata = mConnector.getUnflattenedPos(position);
        id = getChildOrGroupId(posMetadata.position);
        boolean returnValue;
        if (posMetadata.position.type == ExpandableListPosition.GROUP) {
            if (mOnGroupClickListener != null) {
                if (mOnGroupClickListener.onGroupClick(this, v,
                        posMetadata.position.groupPos, id)) {
                    posMetadata.recycle();
                    return true;
                }
            }
            if (posMetadata.isExpanded()) {
                mConnector.collapseGroup(posMetadata);
                playSoundEffect(SoundEffectConstants.CLICK);
                if (mOnGroupCollapseListener != null) {
                    mOnGroupCollapseListener.onGroupCollapse(posMetadata.position.groupPos);
                }
            } else {
                mConnector.expandGroup(posMetadata);
                playSoundEffect(SoundEffectConstants.CLICK);
                if (mOnGroupExpandListener != null) {
                    mOnGroupExpandListener.onGroupExpand(posMetadata.position.groupPos);
                }
                // TODO Make it works on Eclair
                if (VERSION.SDK_INT >= VERSION_CODES.FROYO) {
                    final int groupPos = posMetadata.position.groupPos;
                    final int groupFlatPos = posMetadata.position.flatListPos;
                    final int shiftedGroupPosition = groupFlatPos + getHeaderViewsCount();
                    smoothScrollToPosition(
                            shiftedGroupPosition + mAdapter.getChildrenCount(groupPos),
                            shiftedGroupPosition);
                }
            }
            returnValue = true;
        } else {
            if (mOnChildClickListener != null) {
                playSoundEffect(SoundEffectConstants.CLICK);
                return mOnChildClickListener.onChildClick(this, v, posMetadata.position.groupPos,
                        posMetadata.position.childPos, id);
            }
            returnValue = false;
        }
        posMetadata.recycle();
        return returnValue;
    }

    public boolean isGroupExpanded(int groupPosition) {
        return mConnector.isGroupExpanded(groupPosition);
    }

    private boolean isHeaderOrFooterPosition(int position) {
        final int footerViewsStart = getCount() - getFooterViewsCount();
        return position < getHeaderViewsCount() || position >= footerViewsStart;
    }

    @Override
    public void onInitializeAccessibilityEvent(AccessibilityEvent event) {
        super.onInitializeAccessibilityEvent(event);
        event.setClassName(ExpandableListView.class.getName());
    }

    @Override
    public void onInitializeAccessibilityNodeInfo(AccessibilityNodeInfo info) {
        super.onInitializeAccessibilityNodeInfo(info);
        info.setClassName(ExpandableListView.class.getName());
    }

    @Override
    public void onRestoreInstanceState(Parcelable state) {
        if (!(state instanceof SavedState)) {
            super.onRestoreInstanceState(state);
            return;
        }
        SavedState ss = (SavedState) state;
        super.onRestoreInstanceState(ss.getSuperState());
        if (mConnector != null && ss.expandedGroupMetadataList != null) {
            mConnector.setExpandedGroupMetadataList(ss.expandedGroupMetadataList);
        }
    }

    @Override
    public Parcelable onSaveInstanceState() {
        Parcelable superState = super.onSaveInstanceState();
        return new SavedState(superState,
                mConnector != null ? mConnector.getExpandedGroupMetadataList() : null);
    }

    @Override
    public boolean performItemClick(View v, int position, long id) {
        if (isHeaderOrFooterPosition(position)) {
            return super.performItemClick(v, position, id);
        }
        final int adjustedPosition = getFlatPositionForConnector(position);
        return handleItemClick(v, adjustedPosition, id);
    }

    public void setAdapter(ExpandableListAdapter adapter) {
        mAdapter = adapter;
        if (adapter != null) {
            mConnector = new ExpandableListConnector(adapter);
        } else {
            mConnector = null;
        }
        super.setAdapter(mConnector);
    }

    @Override
    public void setAdapter(ListAdapter adapter) {
        throw new RuntimeException(
                "For ExpandableListView, use setAdapter(ExpandableListAdapter) instead of " +
                        "setAdapter(ListAdapter)");
    }

    public void setChildDivider(Drawable childDivider) {
        mChildDivider = childDivider;
    }

    public void setChildIndicator(Drawable childIndicator) {
        mChildIndicator = childIndicator;
    }

    public void setChildIndicatorBounds(int left, int right) {
        mChildIndicatorLeft = left;
        mChildIndicatorRight = right;
    }

    @Override
    public void setClipToPadding(boolean clipToPadding) {
        super.setClipToPadding(mClipToPadding = clipToPadding);
    }

    public void setGroupIndicator(Drawable groupIndicator) {
        mGroupIndicator = groupIndicator;
        if (mIndicatorRight == 0 && mGroupIndicator != null) {
            mIndicatorRight = mIndicatorLeft + mGroupIndicator.getIntrinsicWidth();
        }
    }

    public void setIndicatorBounds(int left, int right) {
        mIndicatorLeft = left;
        mIndicatorRight = right;
    }

    public void setOnChildClickListener(OnChildClickListener onChildClickListener) {
        mOnChildClickListener = onChildClickListener;
    }

    public void setOnGroupClickListener(OnGroupClickListener onGroupClickListener) {
        mOnGroupClickListener = onGroupClickListener;
    }

    public void setOnGroupCollapseListener(
            OnGroupCollapseListener onGroupCollapseListener) {
        mOnGroupCollapseListener = onGroupCollapseListener;
    }

    public void setOnGroupExpandListener(
            OnGroupExpandListener onGroupExpandListener) {
        mOnGroupExpandListener = onGroupExpandListener;
    }

    @Override
    public void setOnItemClickListener(OnItemClickListener l) {
        super.setOnItemClickListener(l);
    }

    public boolean setSelectedChild(int groupPosition, int childPosition, boolean shouldExpandGroup) {
        ExpandableListPosition elChildPos = ExpandableListPosition.obtainChildPosition(
                groupPosition, childPosition);
        PositionMetadata flatChildPos = mConnector.getFlattenedPos(elChildPos);
        if (flatChildPos == null) {
            if (!shouldExpandGroup) {
                return false;
            }
            expandGroup(groupPosition);
            flatChildPos = mConnector.getFlattenedPos(elChildPos);
            if (flatChildPos == null) {
                throw new IllegalStateException("Could not find child");
            }
        }
        int absoluteFlatPosition = getAbsoluteFlatPosition(flatChildPos.position.flatListPos);
        super.setSelection(absoluteFlatPosition);
        elChildPos.recycle();
        flatChildPos.recycle();
        return true;
    }

    public void setSelectedGroup(int groupPosition) {
        ExpandableListPosition elGroupPos = ExpandableListPosition
                .obtainGroupPosition(groupPosition);
        PositionMetadata pm = mConnector.getFlattenedPos(elGroupPos);
        elGroupPos.recycle();
        final int absoluteFlatPosition = getAbsoluteFlatPosition(pm.position.flatListPos);
        super.setSelection(absoluteFlatPosition);
        pm.recycle();
    }
}
