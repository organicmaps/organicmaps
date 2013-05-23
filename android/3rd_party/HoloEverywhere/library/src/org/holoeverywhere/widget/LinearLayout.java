
package org.holoeverywhere.widget;

import org.holoeverywhere.R;

import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.drawable.Drawable;
import android.os.Build.VERSION;
import android.util.AttributeSet;
import android.view.Gravity;
import android.view.View;
import android.view.ViewDebug;
import android.view.ViewGroup;
import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityNodeInfo;

public class LinearLayout extends android.widget.LinearLayout {
    public static final int HORIZONTAL = 0;
    private static final int INDEX_BOTTOM = 2;
    private static final int INDEX_CENTER_VERTICAL = 0;
    private static final int INDEX_FILL = 3;
    private static final int INDEX_TOP = 1;
    public static final int LAYOUT_DIRECTION_LTR = 0;
    public static final int LAYOUT_DIRECTION_RTL = 1;
    public static final int SHOW_DIVIDER_ALL = 7;
    public static final int SHOW_DIVIDER_BEGINNING = 1;
    public static final int SHOW_DIVIDER_END = 4;
    public static final int SHOW_DIVIDER_MIDDLE = 2;
    public static final int SHOW_DIVIDER_NONE = 0;
    public static final int VERTICAL = 1;
    private static final int VERTICAL_GRAVITY_COUNT = 4;

    public static int getAbsoluteGravity(int gravity, int layoutDirection) {
        int result = gravity;
        if ((result & Gravity.RELATIVE_LAYOUT_DIRECTION) > 0) {
            if ((result & Gravity.START) == Gravity.START) {
                result &= ~Gravity.START;
                if (layoutDirection == LAYOUT_DIRECTION_RTL) {
                    result |= Gravity.RIGHT;
                } else {
                    result |= Gravity.LEFT;
                }
            } else if ((result & Gravity.END) == Gravity.END) {
                result &= ~Gravity.END;
                if (layoutDirection == LAYOUT_DIRECTION_RTL) {
                    result |= Gravity.LEFT;
                } else {
                    result |= Gravity.RIGHT;
                }
            }
            result &= ~Gravity.RELATIVE_LAYOUT_DIRECTION;
        }
        return result;
    }

    public static int resolveSizeAndState(int size, int measureSpec, int childMeasuredState) {
        int result = size;
        int specMode = MeasureSpec.getMode(measureSpec);
        int specSize = MeasureSpec.getSize(measureSpec);
        switch (specMode) {
            case MeasureSpec.UNSPECIFIED:
                result = size;
                break;
            case MeasureSpec.AT_MOST:
                if (specSize < size) {
                    result = specSize | MEASURED_STATE_TOO_SMALL;
                } else {
                    result = size;
                }
                break;
            case MeasureSpec.EXACTLY:
                result = specSize;
                break;
        }
        return result | childMeasuredState & MEASURED_STATE_MASK;
    }

    @ViewDebug.ExportedProperty(category = "layout")
    private boolean mBaselineAligned = true;
    @ViewDebug.ExportedProperty(category = "layout")
    private int mBaselineAlignedChildIndex = -1;
    @ViewDebug.ExportedProperty(category = "measurement")
    private int mBaselineChildTop = 0;
    private Drawable mDivider;
    private int mDividerHeight;
    private int mDividerPadding;
    private int mDividerWidth;
    @ViewDebug.ExportedProperty(category = "measurement", flagMapping = {
            @ViewDebug.FlagToString(mask = -1,
                    equals = -1, name = "NONE"),
            @ViewDebug.FlagToString(mask = Gravity.NO_GRAVITY,
                    equals = Gravity.NO_GRAVITY, name = "NONE"),
            @ViewDebug.FlagToString(mask = Gravity.TOP,
                    equals = Gravity.TOP, name = "TOP"),
            @ViewDebug.FlagToString(mask = Gravity.BOTTOM,
                    equals = Gravity.BOTTOM, name = "BOTTOM"),
            @ViewDebug.FlagToString(mask = Gravity.LEFT,
                    equals = Gravity.LEFT, name = "LEFT"),
            @ViewDebug.FlagToString(mask = Gravity.RIGHT,
                    equals = Gravity.RIGHT, name = "RIGHT"),
            @ViewDebug.FlagToString(mask = Gravity.START,
                    equals = Gravity.START, name = "START"),
            @ViewDebug.FlagToString(mask = Gravity.END,
                    equals = Gravity.END, name = "END"),
            @ViewDebug.FlagToString(mask = Gravity.CENTER_VERTICAL,
                    equals = Gravity.CENTER_VERTICAL, name = "CENTER_VERTICAL"),
            @ViewDebug.FlagToString(mask = Gravity.FILL_VERTICAL,
                    equals = Gravity.FILL_VERTICAL, name = "FILL_VERTICAL"),
            @ViewDebug.FlagToString(mask = Gravity.CENTER_HORIZONTAL,
                    equals = Gravity.CENTER_HORIZONTAL, name = "CENTER_HORIZONTAL"),
            @ViewDebug.FlagToString(mask = Gravity.FILL_HORIZONTAL,
                    equals = Gravity.FILL_HORIZONTAL, name = "FILL_HORIZONTAL"),
            @ViewDebug.FlagToString(mask = Gravity.CENTER,
                    equals = Gravity.CENTER, name = "CENTER"),
            @ViewDebug.FlagToString(mask = Gravity.FILL,
                    equals = Gravity.FILL, name = "FILL"),
            @ViewDebug.FlagToString(mask = Gravity.RELATIVE_LAYOUT_DIRECTION,
                    equals = Gravity.RELATIVE_LAYOUT_DIRECTION, name = "RELATIVE")
    })
    private int mGravity = Gravity.START | Gravity.TOP;
    private int[] mMaxAscent;
    private int[] mMaxDescent;
    @ViewDebug.ExportedProperty(category = "measurement")
    private int mOrientation;
    private int mShowDividers;
    @ViewDebug.ExportedProperty(category = "measurement")
    private int mTotalLength;
    @ViewDebug.ExportedProperty(category = "layout")
    private boolean mUseLargestChild;

    @ViewDebug.ExportedProperty(category = "layout")
    private float mWeightSum;

    public LinearLayout(Context context) {
        this(context, null);
    }

    public LinearLayout(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public LinearLayout(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs);
        TypedArray a = context.obtainStyledAttributes(attrs,
                R.styleable.LinearLayout, defStyle, 0);
        int index = a.getInt(R.styleable.LinearLayout_android_orientation, -1);
        if (index >= 0) {
            setOrientation(index);
        }
        index = a.getInt(R.styleable.LinearLayout_android_gravity, -1);
        if (index >= 0) {
            setGravity(index);
        }
        boolean baselineAligned = a.getBoolean(R.styleable.LinearLayout_android_baselineAligned,
                true);
        if (!baselineAligned) {
            setBaselineAligned(baselineAligned);
        }
        mWeightSum = a.getFloat(R.styleable.LinearLayout_android_weightSum, -1.0f);
        mBaselineAlignedChildIndex = a.getInt(
                R.styleable.LinearLayout_android_baselineAlignedChildIndex, -1);
        mUseLargestChild = a.getBoolean(R.styleable.LinearLayout_android_measureWithLargestChild,
                false);
        setDividerDrawable(a.getDrawable(R.styleable.LinearLayout_android_divider));

        if (a.hasValue(R.styleable.LinearLayout_showDividers)) {
            mShowDividers = a.getInt(R.styleable.LinearLayout_showDividers, SHOW_DIVIDER_NONE);
        } else {
            mShowDividers = a.getInt(R.styleable.LinearLayout_android_showDividers,
                    SHOW_DIVIDER_NONE);
        }

        if (a.hasValue(R.styleable.LinearLayout_dividerPadding)) {
            mDividerPadding = a.getDimensionPixelSize(R.styleable.LinearLayout_dividerPadding, 0);
        } else {
            mDividerPadding = a.getDimensionPixelSize(
                    R.styleable.LinearLayout_android_dividerPadding, 0);
        }

        a.recycle();
    }

    @Override
    protected boolean checkLayoutParams(ViewGroup.LayoutParams p) {
        return p instanceof LinearLayout.LayoutParams;
    }

    void drawDividersHorizontal(Canvas canvas) {
        final int count = getVirtualChildCount();
        final boolean isLayoutRtl = isLayoutRtl();
        for (int i = 0; i < count; i++) {
            final View child = getVirtualChildAt(i);
            if (child != null && child.getVisibility() != GONE) {
                if (hasDividerBeforeChildAt(i)) {
                    final LayoutParams lp = (LayoutParams) child.getLayoutParams();
                    final int position;
                    if (isLayoutRtl) {
                        position = child.getRight() + lp.rightMargin;
                    } else {
                        position = child.getLeft() - lp.leftMargin - mDividerWidth;
                    }
                    drawVerticalDivider(canvas, position);
                }
            }
        }
        if (hasDividerBeforeChildAt(count)) {
            final View child = getVirtualChildAt(count - 1);
            int position;
            if (child == null) {
                if (isLayoutRtl) {
                    position = getPaddingLeft();
                } else {
                    position = getWidth() - getPaddingRight() - mDividerWidth;
                }
            } else {
                final LayoutParams lp = (LayoutParams) child.getLayoutParams();
                if (isLayoutRtl) {
                    position = child.getLeft() - lp.leftMargin - mDividerWidth;
                } else {
                    position = child.getRight() + lp.rightMargin;
                }
            }
            drawVerticalDivider(canvas, position);
        }
    }

    void drawDividersVertical(Canvas canvas) {
        final int count = getVirtualChildCount();
        for (int i = 0; i < count; i++) {
            final View child = getVirtualChildAt(i);
            if (child != null && child.getVisibility() != GONE) {
                if (hasDividerBeforeChildAt(i)) {
                    final LayoutParams lp = (LayoutParams) child.getLayoutParams();
                    final int top = child.getTop() - lp.topMargin - mDividerHeight;
                    drawHorizontalDivider(canvas, top);
                }
            }
        }
        if (hasDividerBeforeChildAt(count)) {
            final View child = getVirtualChildAt(count - 1);
            int bottom = 0;
            if (child == null) {
                bottom = getHeight() - getPaddingBottom() - mDividerHeight;
            } else {
                final LayoutParams lp = (LayoutParams) child.getLayoutParams();
                bottom = child.getBottom() + lp.bottomMargin;
            }
            drawHorizontalDivider(canvas, bottom);
        }
    }

    void drawHorizontalDivider(Canvas canvas, int top) {
        mDivider.setBounds(getPaddingLeft() + mDividerPadding, top,
                getWidth() - getPaddingRight() - mDividerPadding, top + mDividerHeight);
        mDivider.draw(canvas);
    }

    void drawVerticalDivider(Canvas canvas, int left) {
        mDivider.setBounds(left, getPaddingTop() + mDividerPadding,
                left + mDividerWidth, getHeight() - getPaddingBottom() - mDividerPadding);
        mDivider.draw(canvas);
    }

    private void forceUniformHeight(int count, int widthMeasureSpec) {
        int uniformMeasureSpec = MeasureSpec.makeMeasureSpec(getMeasuredHeight(),
                MeasureSpec.EXACTLY);
        for (int i = 0; i < count; ++i) {
            final View child = getVirtualChildAt(i);
            if (child.getVisibility() != GONE) {
                LinearLayout.LayoutParams lp = (LinearLayout.LayoutParams) child.getLayoutParams();
                if (lp.height == android.view.ViewGroup.LayoutParams.MATCH_PARENT) {
                    int oldWidth = lp.width;
                    lp.width = child.getMeasuredWidth();
                    measureChildWithMargins(child, widthMeasureSpec, 0, uniformMeasureSpec, 0);
                    lp.width = oldWidth;
                }
            }
        }
    }

    private void forceUniformWidth(int count, int heightMeasureSpec) {
        int uniformMeasureSpec = MeasureSpec.makeMeasureSpec(getMeasuredWidth(),
                MeasureSpec.EXACTLY);
        for (int i = 0; i < count; ++i) {
            final View child = getVirtualChildAt(i);
            if (child.getVisibility() != GONE) {
                LinearLayout.LayoutParams lp = (LinearLayout.LayoutParams) child.getLayoutParams();
                if (lp.width == android.view.ViewGroup.LayoutParams.MATCH_PARENT) {
                    int oldHeight = lp.height;
                    lp.height = child.getMeasuredHeight();
                    measureChildWithMargins(child, uniformMeasureSpec, 0, heightMeasureSpec, 0);
                    lp.height = oldHeight;
                }
            }
        }
    }

    @Override
    protected LayoutParams generateDefaultLayoutParams() {
        if (mOrientation == HORIZONTAL) {
            return new LayoutParams(android.view.ViewGroup.LayoutParams.WRAP_CONTENT,
                    android.view.ViewGroup.LayoutParams.WRAP_CONTENT);
        } else if (mOrientation == VERTICAL) {
            return new LayoutParams(android.view.ViewGroup.LayoutParams.MATCH_PARENT,
                    android.view.ViewGroup.LayoutParams.WRAP_CONTENT);
        }
        return null;
    }

    @Override
    public LayoutParams generateLayoutParams(AttributeSet attrs) {
        return new LinearLayout.LayoutParams(getContext(), attrs);
    }

    @Override
    protected LayoutParams generateLayoutParams(ViewGroup.LayoutParams p) {
        return new LayoutParams(p);
    }

    @Override
    public int getBaseline() {
        if (mBaselineAlignedChildIndex < 0) {
            return super.getBaseline();
        }
        if (getChildCount() <= mBaselineAlignedChildIndex) {
            throw new RuntimeException("mBaselineAlignedChildIndex of LinearLayout "
                    + "set to an index that is out of bounds.");
        }
        final View child = getChildAt(mBaselineAlignedChildIndex);
        final int childBaseline = child.getBaseline();
        if (childBaseline == -1) {
            if (mBaselineAlignedChildIndex == 0) {
                return -1;
            }
            throw new RuntimeException("mBaselineAlignedChildIndex of LinearLayout "
                    + "points to a View that doesn't know how to get its baseline.");
        }
        int childTop = mBaselineChildTop;
        if (mOrientation == VERTICAL) {
            final int majorGravity = mGravity & Gravity.VERTICAL_GRAVITY_MASK;
            if (majorGravity != Gravity.TOP) {
                switch (majorGravity) {
                    case Gravity.BOTTOM:
                        childTop = getBottom() - getTop() - getPaddingBottom() - mTotalLength;
                        break;
                    case Gravity.CENTER_VERTICAL:
                        childTop += (getBottom() - getTop() - getPaddingTop() - getPaddingBottom() -
                                mTotalLength) / 2;
                        break;
                }
            }
        }
        LinearLayout.LayoutParams lp = (LinearLayout.LayoutParams) child.getLayoutParams();
        return childTop + lp.topMargin + childBaseline;
    }

    @Override
    public int getBaselineAlignedChildIndex() {
        return mBaselineAlignedChildIndex;
    }

    int getChildrenSkipCount(View child, int index) {
        return 0;
    }

    @Override
    public Drawable getDividerDrawable() {
        return mDivider;
    }

    @Override
    public int getDividerPadding() {
        return mDividerPadding;
    }

    public int getDividerWidth() {
        return mDividerWidth;
    }

    public int getLayoutDirection() {
        return LAYOUT_DIRECTION_LTR;
    }

    int getLocationOffset(View child) {
        return 0;
    }

    int getNextLocationOffset(View child) {
        return 0;
    }

    @Override
    public int getOrientation() {
        return mOrientation;
    }

    @Override
    public int getShowDividers() {
        return mShowDividers;
    }

    View getVirtualChildAt(int index) {
        return getChildAt(index);
    }

    int getVirtualChildCount() {
        return getChildCount();
    }

    @Override
    public float getWeightSum() {
        return mWeightSum;
    }

    protected boolean hasDividerBeforeChildAt(int childIndex) {
        if (childIndex == 0) {
            return (mShowDividers & SHOW_DIVIDER_BEGINNING) != 0;
        } else if (childIndex == getChildCount()) {
            return (mShowDividers & SHOW_DIVIDER_END) != 0;
        } else if ((mShowDividers & SHOW_DIVIDER_MIDDLE) != 0) {
            boolean hasVisibleViewBefore = false;
            for (int i = childIndex - 1; i >= 0; i--) {
                if (getChildAt(i).getVisibility() != GONE) {
                    hasVisibleViewBefore = true;
                    break;
                }
            }
            return hasVisibleViewBefore;
        }
        return false;
    }

    @Override
    public boolean isBaselineAligned() {
        return mBaselineAligned;
    }

    protected boolean isLayoutRtl() {
        return getLayoutDirection() == LAYOUT_DIRECTION_RTL;
    }

    @Override
    public boolean isMeasureWithLargestChildEnabled() {
        return mUseLargestChild;
    }

    void layoutHorizontal() {
        final boolean isLayoutRtl = isLayoutRtl();
        final int paddingTop = getPaddingTop();
        int childTop;
        int childLeft;
        final int height = getBottom() - getTop();
        int childBottom = height - getPaddingBottom();
        int childSpace = height - paddingTop - getPaddingBottom();
        final int count = getVirtualChildCount();
        final int majorGravity = mGravity & Gravity.RELATIVE_HORIZONTAL_GRAVITY_MASK;
        final int minorGravity = mGravity & Gravity.VERTICAL_GRAVITY_MASK;
        final boolean baselineAligned = mBaselineAligned;
        final int[] maxAscent = mMaxAscent;
        final int[] maxDescent = mMaxDescent;
        final int layoutDirection = getLayoutDirection();
        switch (getAbsoluteGravity(majorGravity, layoutDirection)) {
            case Gravity.RIGHT:
                childLeft = getPaddingLeft() + getRight() - getLeft() - mTotalLength;
                break;
            case Gravity.CENTER_HORIZONTAL:
                childLeft = getPaddingLeft() + (getRight() - getLeft() - mTotalLength) / 2;
                break;
            case Gravity.LEFT:
            default:
                childLeft = getPaddingLeft();
                break;
        }
        int start = 0;
        int dir = 1;
        if (isLayoutRtl) {
            start = count - 1;
            dir = -1;
        }
        for (int i = 0; i < count; i++) {
            int childIndex = start + dir * i;
            final View child = getVirtualChildAt(childIndex);
            if (child == null) {
                childLeft += measureNullChild(childIndex);
            } else if (child.getVisibility() != GONE) {
                final int childWidth = child.getMeasuredWidth();
                final int childHeight = child.getMeasuredHeight();
                int childBaseline = -1;
                final LinearLayout.LayoutParams lp =
                        (LinearLayout.LayoutParams) child.getLayoutParams();
                if (baselineAligned
                        && lp.height != android.view.ViewGroup.LayoutParams.MATCH_PARENT) {
                    childBaseline = child.getBaseline();
                }
                int gravity = lp.gravity;
                if (gravity < 0) {
                    gravity = minorGravity;
                }
                switch (gravity & Gravity.VERTICAL_GRAVITY_MASK) {
                    case Gravity.TOP:
                        childTop = paddingTop + lp.topMargin;
                        if (childBaseline != -1) {
                            childTop += maxAscent[INDEX_TOP] - childBaseline;
                        }
                        break;
                    case Gravity.CENTER_VERTICAL:
                        childTop = paddingTop + (childSpace - childHeight) / 2
                                + lp.topMargin - lp.bottomMargin;
                        break;
                    case Gravity.BOTTOM:
                        childTop = childBottom - childHeight - lp.bottomMargin;
                        if (childBaseline != -1) {
                            int descent = child.getMeasuredHeight() - childBaseline;
                            childTop -= maxDescent[INDEX_BOTTOM] - descent;
                        }
                        break;
                    default:
                        childTop = paddingTop;
                        break;
                }
                if (hasDividerBeforeChildAt(childIndex)) {
                    childLeft += mDividerWidth;
                }
                childLeft += lp.leftMargin;
                setChildFrame(child, childLeft + getLocationOffset(child), childTop,
                        childWidth, childHeight);
                childLeft += childWidth + lp.rightMargin +
                        getNextLocationOffset(child);
                i += getChildrenSkipCount(child, childIndex);
            }
        }
    }

    void layoutVertical() {
        final int paddingLeft = getPaddingLeft();
        int childTop;
        int childLeft;
        final int width = getRight() - getLeft();
        int childRight = width - getPaddingRight();
        int childSpace = width - paddingLeft - getPaddingRight();
        final int count = getVirtualChildCount();
        final int majorGravity = mGravity & Gravity.VERTICAL_GRAVITY_MASK;
        final int minorGravity = mGravity & Gravity.RELATIVE_HORIZONTAL_GRAVITY_MASK;
        switch (majorGravity) {
            case Gravity.BOTTOM:
                childTop = getPaddingTop() + getBottom() - getTop() - mTotalLength;
                break;
            case Gravity.CENTER_VERTICAL:
                childTop = getPaddingTop() + (getBottom() - getTop() - mTotalLength) / 2;
                break;
            case Gravity.TOP:
            default:
                childTop = getPaddingTop();
                break;
        }
        for (int i = 0; i < count; i++) {
            final View child = getVirtualChildAt(i);
            if (child == null) {
                childTop += measureNullChild(i);
            } else if (child.getVisibility() != GONE) {
                final int childWidth = child.getMeasuredWidth();
                final int childHeight = child.getMeasuredHeight();
                final LinearLayout.LayoutParams lp =
                        (LinearLayout.LayoutParams) child.getLayoutParams();
                int gravity = lp.gravity;
                if (gravity < 0) {
                    gravity = minorGravity;
                }
                final int layoutDirection = getLayoutDirection();
                final int absoluteGravity = getAbsoluteGravity(gravity, layoutDirection);
                switch (absoluteGravity & Gravity.HORIZONTAL_GRAVITY_MASK) {
                    case Gravity.CENTER_HORIZONTAL:
                        childLeft = paddingLeft + (childSpace - childWidth) / 2
                                + lp.leftMargin - lp.rightMargin;
                        break;
                    case Gravity.RIGHT:
                        childLeft = childRight - childWidth - lp.rightMargin;
                        break;
                    case Gravity.LEFT:
                    default:
                        childLeft = paddingLeft + lp.leftMargin;
                        break;
                }
                if (hasDividerBeforeChildAt(i)) {
                    childTop += mDividerHeight;
                }
                childTop += lp.topMargin;
                setChildFrame(child, childLeft, childTop + getLocationOffset(child),
                        childWidth, childHeight);
                childTop += childHeight + lp.bottomMargin + getNextLocationOffset(child);
                i += getChildrenSkipCount(child, i);
            }
        }
    }

    void measureChildBeforeLayout(View child, int childIndex,
            int widthMeasureSpec, int totalWidth, int heightMeasureSpec,
            int totalHeight) {
        measureChildWithMargins(child, widthMeasureSpec, totalWidth,
                heightMeasureSpec, totalHeight);
    }

    void measureHorizontal(int widthMeasureSpec, int heightMeasureSpec) {
        mTotalLength = 0;
        int maxHeight = 0;
        int childState = 0;
        int alternativeMaxHeight = 0;
        int weightedMaxHeight = 0;
        boolean allFillParent = true;
        float totalWeight = 0;
        final int count = getVirtualChildCount();
        final int widthMode = MeasureSpec.getMode(widthMeasureSpec);
        final int heightMode = MeasureSpec.getMode(heightMeasureSpec);
        boolean matchHeight = false;
        if (mMaxAscent == null || mMaxDescent == null) {
            mMaxAscent = new int[VERTICAL_GRAVITY_COUNT];
            mMaxDescent = new int[VERTICAL_GRAVITY_COUNT];
        }
        final int[] maxAscent = mMaxAscent;
        final int[] maxDescent = mMaxDescent;
        maxAscent[0] = maxAscent[1] = maxAscent[2] = maxAscent[3] = -1;
        maxDescent[0] = maxDescent[1] = maxDescent[2] = maxDescent[3] = -1;
        final boolean baselineAligned = mBaselineAligned;
        final boolean useLargestChild = mUseLargestChild;
        final boolean isExactly = widthMode == MeasureSpec.EXACTLY;
        int largestChildWidth = Integer.MIN_VALUE;
        for (int i = 0; i < count; ++i) {
            final View child = getVirtualChildAt(i);
            if (child == null) {
                mTotalLength += measureNullChild(i);
                continue;
            }
            if (child.getVisibility() == GONE) {
                i += getChildrenSkipCount(child, i);
                continue;
            }
            if (hasDividerBeforeChildAt(i)) {
                mTotalLength += mDividerWidth;
            }
            final LinearLayout.LayoutParams lp = (LinearLayout.LayoutParams)
                    child.getLayoutParams();
            totalWeight += lp.weight;
            if (widthMode == MeasureSpec.EXACTLY && lp.width == 0 && lp.weight > 0) {
                if (isExactly) {
                    mTotalLength += lp.leftMargin + lp.rightMargin;
                } else {
                    final int totalLength = mTotalLength;
                    mTotalLength = Math.max(totalLength, totalLength +
                            lp.leftMargin + lp.rightMargin);
                }
                if (baselineAligned) {
                    final int freeSpec = MeasureSpec.makeMeasureSpec(0, MeasureSpec.UNSPECIFIED);
                    child.measure(freeSpec, freeSpec);
                }
            } else {
                int oldWidth = Integer.MIN_VALUE;
                if (lp.width == 0 && lp.weight > 0) {
                    oldWidth = 0;
                    lp.width = android.view.ViewGroup.LayoutParams.WRAP_CONTENT;
                }
                measureChildBeforeLayout(child, i, widthMeasureSpec,
                        totalWeight == 0 ? mTotalLength : 0,
                        heightMeasureSpec, 0);
                if (oldWidth != Integer.MIN_VALUE) {
                    lp.width = oldWidth;
                }
                final int childWidth = child.getMeasuredWidth();
                if (isExactly) {
                    mTotalLength += childWidth + lp.leftMargin + lp.rightMargin +
                            getNextLocationOffset(child);
                } else {
                    final int totalLength = mTotalLength;
                    mTotalLength = Math.max(totalLength, totalLength + childWidth + lp.leftMargin +
                            lp.rightMargin + getNextLocationOffset(child));
                }
                if (useLargestChild) {
                    largestChildWidth = Math.max(childWidth, largestChildWidth);
                }
            }
            boolean matchHeightLocally = false;
            if (heightMode != MeasureSpec.EXACTLY
                    && lp.height == android.view.ViewGroup.LayoutParams.MATCH_PARENT) {
                matchHeight = true;
                matchHeightLocally = true;
            }
            final int margin = lp.topMargin + lp.bottomMargin;
            final int childHeight = child.getMeasuredHeight() + margin;
            if (VERSION.SDK_INT >= 11) {
                childState |= child.getMeasuredState();
            }
            if (baselineAligned) {
                final int childBaseline = child.getBaseline();
                if (childBaseline != -1) {
                    final int gravity = (lp.gravity < 0 ? mGravity : lp.gravity)
                            & Gravity.VERTICAL_GRAVITY_MASK;
                    final int index = (gravity >> Gravity.AXIS_Y_SHIFT
                            & ~Gravity.AXIS_SPECIFIED) >> 1;
                    maxAscent[index] = Math.max(maxAscent[index], childBaseline);
                    maxDescent[index] = Math.max(maxDescent[index], childHeight - childBaseline);
                }
            }
            maxHeight = Math.max(maxHeight, childHeight);
            allFillParent = allFillParent
                    && lp.height == android.view.ViewGroup.LayoutParams.MATCH_PARENT;
            if (lp.weight > 0) {
                weightedMaxHeight = Math.max(weightedMaxHeight,
                        matchHeightLocally ? margin : childHeight);
            } else {
                alternativeMaxHeight = Math.max(alternativeMaxHeight,
                        matchHeightLocally ? margin : childHeight);
            }
            i += getChildrenSkipCount(child, i);
        }
        if (mTotalLength > 0 && hasDividerBeforeChildAt(count)) {
            mTotalLength += mDividerWidth;
        }
        if (maxAscent[INDEX_TOP] != -1 ||
                maxAscent[INDEX_CENTER_VERTICAL] != -1 ||
                maxAscent[INDEX_BOTTOM] != -1 ||
                maxAscent[INDEX_FILL] != -1) {
            final int ascent = Math.max(maxAscent[INDEX_FILL],
                    Math.max(maxAscent[INDEX_CENTER_VERTICAL],
                            Math.max(maxAscent[INDEX_TOP], maxAscent[INDEX_BOTTOM])));
            final int descent = Math.max(maxDescent[INDEX_FILL],
                    Math.max(maxDescent[INDEX_CENTER_VERTICAL],
                            Math.max(maxDescent[INDEX_TOP], maxDescent[INDEX_BOTTOM])));
            maxHeight = Math.max(maxHeight, ascent + descent);
        }
        if (useLargestChild &&
                (widthMode == MeasureSpec.AT_MOST || widthMode == MeasureSpec.UNSPECIFIED)) {
            mTotalLength = 0;
            for (int i = 0; i < count; ++i) {
                final View child = getVirtualChildAt(i);
                if (child == null) {
                    mTotalLength += measureNullChild(i);
                    continue;
                }
                if (child.getVisibility() == GONE) {
                    i += getChildrenSkipCount(child, i);
                    continue;
                }
                final LinearLayout.LayoutParams lp = (LinearLayout.LayoutParams)
                        child.getLayoutParams();
                if (isExactly) {
                    mTotalLength += largestChildWidth + lp.leftMargin + lp.rightMargin +
                            getNextLocationOffset(child);
                } else {
                    final int totalLength = mTotalLength;
                    mTotalLength = Math.max(totalLength, totalLength + largestChildWidth +
                            lp.leftMargin + lp.rightMargin + getNextLocationOffset(child));
                }
            }
        }
        mTotalLength += getPaddingLeft() + getPaddingRight();
        int widthSize = mTotalLength;
        widthSize = Math.max(widthSize, getSuggestedMinimumWidth());
        int widthSizeAndState = resolveSizeAndState(widthSize, widthMeasureSpec, 0);
        widthSize = widthSizeAndState & MEASURED_SIZE_MASK;
        int delta = widthSize - mTotalLength;
        if (delta != 0 && totalWeight > 0.0f) {
            float weightSum = mWeightSum > 0.0f ? mWeightSum : totalWeight;
            maxAscent[0] = maxAscent[1] = maxAscent[2] = maxAscent[3] = -1;
            maxDescent[0] = maxDescent[1] = maxDescent[2] = maxDescent[3] = -1;
            maxHeight = -1;
            mTotalLength = 0;
            for (int i = 0; i < count; ++i) {
                final View child = getVirtualChildAt(i);
                if (child == null || child.getVisibility() == View.GONE) {
                    continue;
                }
                final LinearLayout.LayoutParams lp =
                        (LinearLayout.LayoutParams) child.getLayoutParams();
                float childExtra = lp.weight;
                if (childExtra > 0) {
                    int share = (int) (childExtra * delta / weightSum);
                    weightSum -= childExtra;
                    delta -= share;
                    final int childHeightMeasureSpec = getChildMeasureSpec(
                            heightMeasureSpec, getPaddingTop() + getPaddingBottom() + lp.topMargin
                                    + lp.bottomMargin,
                            lp.height);
                    if (lp.width != 0 || widthMode != MeasureSpec.EXACTLY) {
                        int childWidth = child.getMeasuredWidth() + share;
                        if (childWidth < 0) {
                            childWidth = 0;
                        }
                        child.measure(
                                MeasureSpec.makeMeasureSpec(childWidth, MeasureSpec.EXACTLY),
                                childHeightMeasureSpec);
                    } else {
                        child.measure(MeasureSpec.makeMeasureSpec(
                                share > 0 ? share : 0, MeasureSpec.EXACTLY),
                                childHeightMeasureSpec);
                    }
                    if (VERSION.SDK_INT >= 11) {
                        childState |= child.getMeasuredState() & MEASURED_STATE_MASK;
                    }
                }
                if (isExactly) {
                    mTotalLength += child.getMeasuredWidth() + lp.leftMargin + lp.rightMargin +
                            getNextLocationOffset(child);
                } else {
                    final int totalLength = mTotalLength;
                    mTotalLength = Math.max(totalLength, totalLength + child.getMeasuredWidth() +
                            lp.leftMargin + lp.rightMargin + getNextLocationOffset(child));
                }
                boolean matchHeightLocally = heightMode != MeasureSpec.EXACTLY &&
                        lp.height == android.view.ViewGroup.LayoutParams.MATCH_PARENT;
                final int margin = lp.topMargin + lp.bottomMargin;
                int childHeight = child.getMeasuredHeight() + margin;
                maxHeight = Math.max(maxHeight, childHeight);
                alternativeMaxHeight = Math.max(alternativeMaxHeight,
                        matchHeightLocally ? margin : childHeight);
                allFillParent = allFillParent
                        && lp.height == android.view.ViewGroup.LayoutParams.MATCH_PARENT;
                if (baselineAligned) {
                    final int childBaseline = child.getBaseline();
                    if (childBaseline != -1) {
                        final int gravity = (lp.gravity < 0 ? mGravity : lp.gravity)
                                & Gravity.VERTICAL_GRAVITY_MASK;
                        final int index = (gravity >> Gravity.AXIS_Y_SHIFT
                                & ~Gravity.AXIS_SPECIFIED) >> 1;
                        maxAscent[index] = Math.max(maxAscent[index], childBaseline);
                        maxDescent[index] = Math.max(maxDescent[index],
                                childHeight - childBaseline);
                    }
                }
            }
            mTotalLength += getPaddingLeft() + getPaddingRight();
            if (maxAscent[INDEX_TOP] != -1 ||
                    maxAscent[INDEX_CENTER_VERTICAL] != -1 ||
                    maxAscent[INDEX_BOTTOM] != -1 ||
                    maxAscent[INDEX_FILL] != -1) {
                final int ascent = Math.max(maxAscent[INDEX_FILL],
                        Math.max(maxAscent[INDEX_CENTER_VERTICAL],
                                Math.max(maxAscent[INDEX_TOP], maxAscent[INDEX_BOTTOM])));
                final int descent = Math.max(maxDescent[INDEX_FILL],
                        Math.max(maxDescent[INDEX_CENTER_VERTICAL],
                                Math.max(maxDescent[INDEX_TOP], maxDescent[INDEX_BOTTOM])));
                maxHeight = Math.max(maxHeight, ascent + descent);
            }
        } else {
            alternativeMaxHeight = Math.max(alternativeMaxHeight, weightedMaxHeight);
            if (useLargestChild && widthMode != MeasureSpec.EXACTLY) {
                for (int i = 0; i < count; i++) {
                    final View child = getVirtualChildAt(i);
                    if (child == null || child.getVisibility() == View.GONE) {
                        continue;
                    }
                    final LinearLayout.LayoutParams lp =
                            (LinearLayout.LayoutParams) child.getLayoutParams();
                    float childExtra = lp.weight;
                    if (childExtra > 0) {
                        child.measure(
                                MeasureSpec.makeMeasureSpec(largestChildWidth, MeasureSpec.EXACTLY),
                                MeasureSpec.makeMeasureSpec(child.getMeasuredHeight(),
                                        MeasureSpec.EXACTLY));
                    }
                }
            }
        }
        if (!allFillParent && heightMode != MeasureSpec.EXACTLY) {
            maxHeight = alternativeMaxHeight;
        }
        maxHeight += getPaddingTop() + getPaddingBottom();
        maxHeight = Math.max(maxHeight, getSuggestedMinimumHeight());
        setMeasuredDimension(widthSizeAndState | childState & MEASURED_STATE_MASK,
                resolveSizeAndState(maxHeight, heightMeasureSpec,
                        childState << MEASURED_HEIGHT_STATE_SHIFT));
        if (matchHeight) {
            forceUniformHeight(count, widthMeasureSpec);
        }
    }

    int measureNullChild(int childIndex) {
        return 0;
    }

    void measureVertical(int widthMeasureSpec, int heightMeasureSpec) {
        mTotalLength = 0;
        int maxWidth = 0;
        int childState = 0;
        int alternativeMaxWidth = 0;
        int weightedMaxWidth = 0;
        boolean allFillParent = true;
        float totalWeight = 0;
        final int count = getVirtualChildCount();
        final int widthMode = MeasureSpec.getMode(widthMeasureSpec);
        final int heightMode = MeasureSpec.getMode(heightMeasureSpec);
        boolean matchWidth = false;
        final int baselineChildIndex = mBaselineAlignedChildIndex;
        final boolean useLargestChild = mUseLargestChild;
        int largestChildHeight = Integer.MIN_VALUE;
        for (int i = 0; i < count; ++i) {
            final View child = getVirtualChildAt(i);
            if (child == null) {
                mTotalLength += measureNullChild(i);
                continue;
            }
            if (child.getVisibility() == View.GONE) {
                i += getChildrenSkipCount(child, i);
                continue;
            }
            if (hasDividerBeforeChildAt(i)) {
                mTotalLength += mDividerHeight;
            }
            LinearLayout.LayoutParams lp = (LinearLayout.LayoutParams) child.getLayoutParams();
            totalWeight += lp.weight;
            if (heightMode == MeasureSpec.EXACTLY && lp.height == 0 && lp.weight > 0) {
                final int totalLength = mTotalLength;
                mTotalLength = Math.max(totalLength, totalLength + lp.topMargin + lp.bottomMargin);
            } else {
                int oldHeight = Integer.MIN_VALUE;
                if (lp.height == 0 && lp.weight > 0) {
                    oldHeight = 0;
                    lp.height = android.view.ViewGroup.LayoutParams.WRAP_CONTENT;
                }
                measureChildBeforeLayout(
                        child, i, widthMeasureSpec, 0, heightMeasureSpec,
                        totalWeight == 0 ? mTotalLength : 0);
                if (oldHeight != Integer.MIN_VALUE) {
                    lp.height = oldHeight;
                }
                final int childHeight = child.getMeasuredHeight();
                final int totalLength = mTotalLength;
                mTotalLength = Math.max(totalLength, totalLength + childHeight + lp.topMargin +
                        lp.bottomMargin + getNextLocationOffset(child));
                if (useLargestChild) {
                    largestChildHeight = Math.max(childHeight, largestChildHeight);
                }
            }
            if (baselineChildIndex >= 0 && baselineChildIndex == i + 1) {
                mBaselineChildTop = mTotalLength;
            }
            if (i < baselineChildIndex && lp.weight > 0) {
                throw new RuntimeException("A child of LinearLayout with index "
                        + "less than mBaselineAlignedChildIndex has weight > 0, which "
                        + "won't work.  Either remove the weight, or don't set "
                        + "mBaselineAlignedChildIndex.");
            }
            boolean matchWidthLocally = false;
            if (widthMode != MeasureSpec.EXACTLY
                    && lp.width == android.view.ViewGroup.LayoutParams.MATCH_PARENT) {
                matchWidth = true;
                matchWidthLocally = true;
            }
            final int margin = lp.leftMargin + lp.rightMargin;
            final int measuredWidth = child.getMeasuredWidth() + margin;
            maxWidth = Math.max(maxWidth, measuredWidth);
            if (VERSION.SDK_INT >= 11) {
                childState |= child.getMeasuredState();
            }
            allFillParent = allFillParent
                    && lp.width == android.view.ViewGroup.LayoutParams.MATCH_PARENT;
            if (lp.weight > 0) {
                weightedMaxWidth = Math.max(weightedMaxWidth,
                        matchWidthLocally ? margin : measuredWidth);
            } else {
                alternativeMaxWidth = Math.max(alternativeMaxWidth,
                        matchWidthLocally ? margin : measuredWidth);
            }

            i += getChildrenSkipCount(child, i);
        }
        if (mTotalLength > 0 && hasDividerBeforeChildAt(count)) {
            mTotalLength += mDividerHeight;
        }
        if (useLargestChild &&
                (heightMode == MeasureSpec.AT_MOST || heightMode == MeasureSpec.UNSPECIFIED)) {
            mTotalLength = 0;
            for (int i = 0; i < count; ++i) {
                final View child = getVirtualChildAt(i);
                if (child == null) {
                    mTotalLength += measureNullChild(i);
                    continue;
                }
                if (child.getVisibility() == GONE) {
                    i += getChildrenSkipCount(child, i);
                    continue;
                }
                final LinearLayout.LayoutParams lp = (LinearLayout.LayoutParams)
                        child.getLayoutParams();
                final int totalLength = mTotalLength;
                mTotalLength = Math.max(totalLength, totalLength + largestChildHeight +
                        lp.topMargin + lp.bottomMargin + getNextLocationOffset(child));
            }
        }
        mTotalLength += getPaddingTop() + getPaddingBottom();
        int heightSize = mTotalLength;
        heightSize = Math.max(heightSize, getSuggestedMinimumHeight());
        int heightSizeAndState = resolveSizeAndState(heightSize, heightMeasureSpec, 0);
        heightSize = heightSizeAndState & MEASURED_SIZE_MASK;
        int delta = heightSize - mTotalLength;
        if (delta != 0 && totalWeight > 0.0f) {
            float weightSum = mWeightSum > 0.0f ? mWeightSum : totalWeight;
            mTotalLength = 0;
            for (int i = 0; i < count; ++i) {
                final View child = getVirtualChildAt(i);
                if (child.getVisibility() == View.GONE) {
                    continue;
                }
                LinearLayout.LayoutParams lp = (LinearLayout.LayoutParams) child.getLayoutParams();
                float childExtra = lp.weight;
                if (childExtra > 0) {
                    int share = (int) (childExtra * delta / weightSum);
                    weightSum -= childExtra;
                    delta -= share;
                    final int childWidthMeasureSpec = getChildMeasureSpec(widthMeasureSpec,
                            getPaddingLeft() + getPaddingRight() +
                                    lp.leftMargin + lp.rightMargin, lp.width);
                    if (lp.height != 0 || heightMode != MeasureSpec.EXACTLY) {
                        int childHeight = child.getMeasuredHeight() + share;
                        if (childHeight < 0) {
                            childHeight = 0;
                        }
                        child.measure(childWidthMeasureSpec,
                                MeasureSpec.makeMeasureSpec(childHeight, MeasureSpec.EXACTLY));
                    } else {
                        child.measure(childWidthMeasureSpec,
                                MeasureSpec.makeMeasureSpec(share > 0 ? share : 0,
                                        MeasureSpec.EXACTLY));
                    }
                    if (VERSION.SDK_INT >= 11) {
                        childState |= child.getMeasuredState()
                                & MEASURED_STATE_MASK >> MEASURED_HEIGHT_STATE_SHIFT;
                    }
                }
                final int margin = lp.leftMargin + lp.rightMargin;
                final int measuredWidth = child.getMeasuredWidth() + margin;
                maxWidth = Math.max(maxWidth, measuredWidth);
                boolean matchWidthLocally = widthMode != MeasureSpec.EXACTLY &&
                        lp.width == android.view.ViewGroup.LayoutParams.MATCH_PARENT;
                alternativeMaxWidth = Math.max(alternativeMaxWidth,
                        matchWidthLocally ? margin : measuredWidth);
                allFillParent = allFillParent
                        && lp.width == android.view.ViewGroup.LayoutParams.MATCH_PARENT;
                final int totalLength = mTotalLength;
                mTotalLength = Math.max(totalLength, totalLength + child.getMeasuredHeight() +
                        lp.topMargin + lp.bottomMargin + getNextLocationOffset(child));
            }
            mTotalLength += getPaddingTop() + getPaddingBottom();
        } else {
            alternativeMaxWidth = Math.max(alternativeMaxWidth,
                    weightedMaxWidth);
            if (useLargestChild && heightMode != MeasureSpec.EXACTLY) {
                for (int i = 0; i < count; i++) {
                    final View child = getVirtualChildAt(i);
                    if (child == null || child.getVisibility() == View.GONE) {
                        continue;
                    }
                    final LinearLayout.LayoutParams lp =
                            (LinearLayout.LayoutParams) child.getLayoutParams();
                    float childExtra = lp.weight;
                    if (childExtra > 0) {
                        child.measure(MeasureSpec.makeMeasureSpec(child.getMeasuredWidth(),
                                MeasureSpec.EXACTLY), MeasureSpec.makeMeasureSpec(
                                largestChildHeight, MeasureSpec.EXACTLY));
                    }
                }
            }
        }
        if (!allFillParent && widthMode != MeasureSpec.EXACTLY) {
            maxWidth = alternativeMaxWidth;
        }
        maxWidth += getPaddingLeft() + getPaddingRight();
        maxWidth = Math.max(maxWidth, getSuggestedMinimumWidth());
        setMeasuredDimension(resolveSizeAndState(maxWidth, widthMeasureSpec, childState),
                heightSizeAndState);
        if (matchWidth) {
            forceUniformWidth(count, heightMeasureSpec);
        }
    }

    @Override
    protected void onDraw(Canvas canvas) {
        if (mDivider == null) {
            return;
        }
        if (mOrientation == VERTICAL) {
            drawDividersVertical(canvas);
        } else {
            drawDividersHorizontal(canvas);
        }
    }

    @Override
    public void onInitializeAccessibilityEvent(AccessibilityEvent event) {
        super.onInitializeAccessibilityEvent(event);
        event.setClassName(LinearLayout.class.getName());
    }

    @Override
    public void onInitializeAccessibilityNodeInfo(AccessibilityNodeInfo info) {
        super.onInitializeAccessibilityNodeInfo(info);
        info.setClassName(LinearLayout.class.getName());
    }

    @Override
    protected void onLayout(boolean changed, int l, int t, int r, int b) {
        if (mOrientation == VERTICAL) {
            layoutVertical();
        } else {
            layoutHorizontal();
        }
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        if (mOrientation == VERTICAL) {
            measureVertical(widthMeasureSpec, heightMeasureSpec);
        } else {
            measureHorizontal(widthMeasureSpec, heightMeasureSpec);
        }
    }

    @Override
    public void setBaselineAligned(boolean baselineAligned) {
        mBaselineAligned = baselineAligned;
    }

    @Override
    public void setBaselineAlignedChildIndex(int i) {
        if (i < 0 || i >= getChildCount()) {
            throw new IllegalArgumentException("base aligned child index out "
                    + "of range (0, " + getChildCount() + ")");
        }
        mBaselineAlignedChildIndex = i;
    }

    private void setChildFrame(View child, int left, int top, int width, int height) {
        child.layout(left, top, left + width, top + height);
    }

    @Override
    public void setDividerDrawable(Drawable divider) {
        if (divider == mDivider) {
            return;
        }
        mDivider = divider;
        if (divider != null) {
            mDividerWidth = divider.getIntrinsicWidth();
            mDividerHeight = divider.getIntrinsicHeight();
        } else {
            mDividerWidth = 0;
            mDividerHeight = 0;
        }
        setWillNotDraw(divider == null);
        requestLayout();
    }

    @Override
    public void setDividerPadding(int padding) {
        mDividerPadding = padding;
    }

    @Override
    public void setGravity(int gravity) {
        if (mGravity != gravity) {
            if ((gravity & Gravity.RELATIVE_HORIZONTAL_GRAVITY_MASK) == 0) {
                gravity |= Gravity.START;
            }
            if ((gravity & Gravity.VERTICAL_GRAVITY_MASK) == 0) {
                gravity |= Gravity.TOP;
            }
            mGravity = gravity;
            requestLayout();
        }
    }

    @Override
    public void setHorizontalGravity(int horizontalGravity) {
        final int gravity = horizontalGravity & Gravity.RELATIVE_HORIZONTAL_GRAVITY_MASK;
        if ((mGravity & Gravity.RELATIVE_HORIZONTAL_GRAVITY_MASK) != gravity) {
            mGravity = mGravity & ~Gravity.RELATIVE_HORIZONTAL_GRAVITY_MASK | gravity;
            requestLayout();
        }
    }

    @Override
    public void setMeasureWithLargestChildEnabled(boolean enabled) {
        mUseLargestChild = enabled;
    }

    @Override
    public void setOrientation(int orientation) {
        if (mOrientation != orientation) {
            mOrientation = orientation;
            requestLayout();
        }
    }

    @Override
    public void setShowDividers(int showDividers) {
        if (showDividers != mShowDividers) {
            requestLayout();
        }
        mShowDividers = showDividers;
    }

    @Override
    public void setVerticalGravity(int verticalGravity) {
        final int gravity = verticalGravity & Gravity.VERTICAL_GRAVITY_MASK;
        if ((mGravity & Gravity.VERTICAL_GRAVITY_MASK) != gravity) {
            mGravity = mGravity & ~Gravity.VERTICAL_GRAVITY_MASK | gravity;
            requestLayout();
        }
    }

    @Override
    public void setWeightSum(float weightSum) {
        mWeightSum = Math.max(0.0f, weightSum);
    }

    @Override
    public boolean shouldDelayChildPressedState() {
        return false;
    }
}
