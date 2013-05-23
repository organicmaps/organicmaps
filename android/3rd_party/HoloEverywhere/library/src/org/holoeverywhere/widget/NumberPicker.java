
package org.holoeverywhere.widget;

import java.util.ArrayList;

import org.holoeverywhere.LayoutInflater;
import org.holoeverywhere.R;
import org.holoeverywhere.internal.NumberPickerEditText;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.res.ColorStateList;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Paint.Align;
import android.graphics.drawable.Drawable;
import android.os.Build.VERSION;
import android.text.InputFilter;
import android.text.InputType;
import android.text.Spanned;
import android.text.TextUtils;
import android.text.method.NumberKeyListener;
import android.util.AttributeSet;
import android.util.SparseArray;
import android.util.TypedValue;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.VelocityTracker;
import android.view.View;
import android.view.ViewConfiguration;
import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityManager;
import android.view.accessibility.AccessibilityNodeInfo;
import android.view.animation.DecelerateInterpolator;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.ImageButton;

public class NumberPicker extends LinearLayout {
    class BeginSoftInputOnLongPressCommand implements Runnable {
        @Override
        public void run() {
            showSoftInput();
            mIngonreMoveEvents = true;
        }
    }

    class ChangeCurrentByOneFromLongPressCommand implements Runnable {
        private boolean mIncrement;

        @Override
        public void run() {
            changeValueByOne(mIncrement);
            postDelayed(this, mLongPressUpdateInterval);
        }

        private void setStep(boolean increment) {
            mIncrement = increment;
        }
    }

    public interface Formatter {
        public String format(int value);
    }

    class InputTextFilter extends NumberKeyListener {
        @Override
        public CharSequence filter(CharSequence source, int start, int end,
                Spanned dest, int dstart, int dend) {
            if (mDisplayedValues == null) {
                CharSequence filtered = super.filter(source, start, end, dest,
                        dstart, dend);
                if (filtered == null) {
                    filtered = source.subSequence(start, end);
                }
                String result = String.valueOf(dest.subSequence(0, dstart))
                        + filtered + dest.subSequence(dend, dest.length());

                if ("".equals(result)) {
                    return result;
                }
                int val = getSelectedPos(result);
                if (val > mMaxValue) {
                    return "";
                } else {
                    return filtered;
                }
            } else {
                CharSequence filtered = String.valueOf(source.subSequence(
                        start, end));
                if (TextUtils.isEmpty(filtered)) {
                    return "";
                }
                String result = String.valueOf(dest.subSequence(0, dstart))
                        + filtered + dest.subSequence(dend, dest.length());
                String str = String.valueOf(result).toLowerCase();
                for (String val : mDisplayedValues) {
                    String valLowerCase = val.toLowerCase();
                    if (valLowerCase.startsWith(str)) {
                        postSetSelectionCommand(result.length(), val.length());
                        return val.subSequence(dstart, val.length());
                    }
                }
                return "";
            }
        }

        @Override
        protected char[] getAcceptedChars() {
            return NumberPicker.DIGIT_CHARACTERS;
        }

        @Override
        public int getInputType() {
            return InputType.TYPE_CLASS_TEXT;
        }
    }

    public interface OnScrollListener {
        public static int SCROLL_STATE_FLING = 2;
        public static int SCROLL_STATE_IDLE = 0;
        public static int SCROLL_STATE_TOUCH_SCROLL = 1;

        public void onScrollStateChange(NumberPicker view, int scrollState);
    }

    public interface OnValueChangeListener {
        void onValueChange(NumberPicker picker, int oldVal, int newVal);
    }

    class PressedStateHelper implements Runnable {
        public static final int BUTTON_DECREMENT = 2;
        public static final int BUTTON_INCREMENT = 1;
        private static final int MODE_PRESS = 1;
        private static final int MODE_TAPPED = 2;
        private int mManagedButton;
        private int mMode;

        public void buttonPressDelayed(int button) {
            cancel();
            mMode = MODE_PRESS;
            mManagedButton = button;
            postDelayed(this, ViewConfiguration.getTapTimeout());
        }

        public void buttonTapped(int button) {
            cancel();
            mMode = MODE_TAPPED;
            mManagedButton = button;
            post(this);
        }

        public void cancel() {
            mMode = 0;
            mManagedButton = 0;
            removeCallbacks(this);
            if (mIncrementVirtualButtonPressed) {
                mIncrementVirtualButtonPressed = false;
                invalidate(0, mBottomSelectionDividerBottom, getRight(),
                        getBottom());
            }
            mDecrementVirtualButtonPressed = false;
            if (mDecrementVirtualButtonPressed) {
                invalidate(0, 0, getRight(), mTopSelectionDividerTop);
            }
        }

        @Override
        public void run() {
            switch (mMode) {
                case MODE_PRESS: {
                    switch (mManagedButton) {
                        case BUTTON_INCREMENT: {
                            mIncrementVirtualButtonPressed = true;
                            invalidate(0, mBottomSelectionDividerBottom, getRight(),
                                    getBottom());
                        }
                            break;
                        case BUTTON_DECREMENT: {
                            mDecrementVirtualButtonPressed = true;
                            invalidate(0, 0, getRight(), mTopSelectionDividerTop);
                        }
                    }
                }
                    break;
                case MODE_TAPPED: {
                    switch (mManagedButton) {
                        case BUTTON_INCREMENT: {
                            if (!mIncrementVirtualButtonPressed) {
                                postDelayed(this,
                                        ViewConfiguration.getPressedStateDuration());
                            }
                            mIncrementVirtualButtonPressed ^= true;
                            invalidate(0, mBottomSelectionDividerBottom, getRight(),
                                    getBottom());
                        }
                            break;
                        case BUTTON_DECREMENT: {
                            if (!mDecrementVirtualButtonPressed) {
                                postDelayed(this,
                                        ViewConfiguration.getPressedStateDuration());
                            }
                            mDecrementVirtualButtonPressed ^= true;
                            invalidate(0, 0, getRight(), mTopSelectionDividerTop);
                        }
                    }
                }
                    break;
            }
        }
    }

    class SetSelectionCommand implements Runnable {
        private int mSelectionEnd;

        private int mSelectionStart;

        @Override
        public void run() {
            mInputText.setSelection(mSelectionStart, mSelectionEnd);
        }
    }

    private static final int DEFAULT_LAYOUT_RESOURCE_ID = R.layout.number_picker_with_selector_wheel;
    private static final long DEFAULT_LONG_PRESS_UPDATE_INTERVAL = 300;
    private static final char[] DIGIT_CHARACTERS = new char[] {
            '0', '1', '2',
            '3', '4', '5', '6', '7', '8', '9'
    };
    public static final int FOCUSABLES_ACCESSIBILITY = 0x00000002;
    private static final int SELECTOR_ADJUSTMENT_DURATION_MILLIS = 800;
    private static final int SELECTOR_MAX_FLING_VELOCITY_ADJUSTMENT = 8;
    private static final int SELECTOR_WHEEL_ITEM_COUNT = 3;
    private static final int SELECTOR_WHELL_MIDDLE_ITEM_INDEX = NumberPicker.SELECTOR_WHEEL_ITEM_COUNT / 2;
    private static final int SIZE_UNSPECIFIED = -1;
    private static final int SNAP_SCROLL_DURATION = 300;
    private static final float TOP_AND_BOTTOM_FADING_EDGE_STRENGTH = 0.9f;
    public static final NumberPicker.Formatter TWO_DIGIT_FORMATTER = new NumberPicker.Formatter() {
        final Object[] mArgs = new Object[1];

        final StringBuilder mBuilder = new StringBuilder();

        final java.util.Formatter mFmt = new java.util.Formatter(mBuilder,
                java.util.Locale.US);

        @Override
        public String format(int value) {
            mArgs[0] = value;
            mBuilder.delete(0, mBuilder.length());
            mFmt.format("%02d", mArgs);
            return mFmt.toString();
        }
    };
    private static final int UNSCALED_DEFAULT_SELECTION_DIVIDER_HEIGHT = 2;
    private static final int UNSCALED_DEFAULT_SELECTION_DIVIDERS_DISTANCE = 48;
    private final Scroller mAdjustScroller;
    private BeginSoftInputOnLongPressCommand mBeginSoftInputOnLongPressCommand;
    private int mBottomSelectionDividerBottom;
    private ChangeCurrentByOneFromLongPressCommand mChangeCurrentByOneFromLongPressCommand;
    private final boolean mComputeMaxWidth;
    private int mCurrentScrollOffset;
    private final ImageButton mDecrementButton;
    private boolean mDecrementVirtualButtonPressed;
    private String[] mDisplayedValues;
    private final Scroller mFlingScroller;
    private Formatter mFormatter;
    private final boolean mHasSelectorWheel;
    private final ImageButton mIncrementButton;
    private boolean mIncrementVirtualButtonPressed;
    private boolean mIngonreMoveEvents;
    private int mInitialScrollOffset = Integer.MIN_VALUE;
    private final NumberPickerEditText mInputText;
    private long mLastDownEventTime;
    private float mLastDownEventY;
    private float mLastDownOrMoveEventY;
    private long mLongPressUpdateInterval = NumberPicker.DEFAULT_LONG_PRESS_UPDATE_INTERVAL;
    private final int mMaxHeight;
    private int mMaximumFlingVelocity;
    private int mMaxValue;
    private int mMaxWidth;
    private final int mMinHeight;
    private int mMinimumFlingVelocity;
    private int mMinValue;
    private final int mMinWidth;
    private OnScrollListener mOnScrollListener;
    private OnValueChangeListener mOnValueChangeListener;
    private final PressedStateHelper mPressedStateHelper;
    private int mPreviousScrollerY;
    private int mScrollState = OnScrollListener.SCROLL_STATE_IDLE;
    private final Drawable mSelectionDivider;
    private final int mSelectionDividerHeight;
    private final int mSelectionDividersDistance;
    private int mSelectorElementHeight;
    private final SparseArray<String> mSelectorIndexToStringCache = new SparseArray<String>();
    private final int[] mSelectorIndices = new int[NumberPicker.SELECTOR_WHEEL_ITEM_COUNT];
    private int mSelectorTextGapHeight;
    private final Paint mSelectorWheelPaint;
    private SetSelectionCommand mSetSelectionCommand;
    private boolean mShowSoftInputOnTap;
    private final int mSolidColor;
    private final int mTextSize;
    private int mTopSelectionDividerTop;
    private int mTouchSlop;
    private int mValue;
    private VelocityTracker mVelocityTracker;
    private final Drawable mVirtualButtonPressedDrawable;
    private boolean mWrapSelectorWheel;

    public NumberPicker(Context context) {
        this(context, null);
    }

    public NumberPicker(Context context, AttributeSet attrs) {
        this(context, attrs, R.attr.numberPickerStyle);
    }

    @SuppressLint("NewApi")
    public NumberPicker(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        TypedArray attributesArray = context.obtainStyledAttributes(attrs,
                R.styleable.NumberPicker, defStyle, R.style.Holo_NumberPicker);
        final int layoutResId = attributesArray.getResourceId(
                R.styleable.NumberPicker_android_layout,
                NumberPicker.DEFAULT_LAYOUT_RESOURCE_ID);
        mHasSelectorWheel = layoutResId == NumberPicker.DEFAULT_LAYOUT_RESOURCE_ID;
        mSolidColor = attributesArray.getColor(
                R.styleable.NumberPicker_solidColor, 0);
        mSelectionDivider = attributesArray
                .getDrawable(R.styleable.NumberPicker_selectionDivider);
        final int defSelectionDividerHeight = (int) TypedValue.applyDimension(
                TypedValue.COMPLEX_UNIT_DIP,
                NumberPicker.UNSCALED_DEFAULT_SELECTION_DIVIDER_HEIGHT,
                getResources().getDisplayMetrics());
        mSelectionDividerHeight = attributesArray.getDimensionPixelSize(
                R.styleable.NumberPicker_selectionDividerHeight,
                defSelectionDividerHeight);
        final int defSelectionDividerDistance = (int) TypedValue
                .applyDimension(
                        TypedValue.COMPLEX_UNIT_DIP,
                        NumberPicker.UNSCALED_DEFAULT_SELECTION_DIVIDERS_DISTANCE,
                        getResources().getDisplayMetrics());
        mSelectionDividersDistance = attributesArray.getDimensionPixelSize(
                R.styleable.NumberPicker_selectionDividersDistance,
                defSelectionDividerDistance);
        mMinHeight = attributesArray.getDimensionPixelSize(
                R.styleable.NumberPicker_android_minHeight,
                NumberPicker.SIZE_UNSPECIFIED);
        mMaxHeight = attributesArray.getDimensionPixelSize(
                R.styleable.NumberPicker_android_maxHeight,
                NumberPicker.SIZE_UNSPECIFIED);
        if (mMinHeight != NumberPicker.SIZE_UNSPECIFIED
                && mMaxHeight != NumberPicker.SIZE_UNSPECIFIED
                && mMinHeight > mMaxHeight) {
            throw new IllegalArgumentException("minHeight > maxHeight");
        }
        mMinWidth = attributesArray.getDimensionPixelSize(
                R.styleable.NumberPicker_android_minWidth,
                NumberPicker.SIZE_UNSPECIFIED);
        mMaxWidth = attributesArray.getDimensionPixelSize(
                R.styleable.NumberPicker_android_maxWidth,
                NumberPicker.SIZE_UNSPECIFIED);
        if (mMinWidth != NumberPicker.SIZE_UNSPECIFIED
                && mMaxWidth != NumberPicker.SIZE_UNSPECIFIED
                && mMinWidth > mMaxWidth) {
            throw new IllegalArgumentException("minWidth > maxWidth");
        }
        mComputeMaxWidth = mMaxWidth == NumberPicker.SIZE_UNSPECIFIED;
        mVirtualButtonPressedDrawable = attributesArray
                .getDrawable(R.styleable.NumberPicker_virtualButtonPressedDrawable);
        attributesArray.recycle();
        mPressedStateHelper = new PressedStateHelper();
        setWillNotDraw(!mHasSelectorWheel);
        LayoutInflater.inflate(context, layoutResId, this, true);
        OnClickListener onClickListener = new OnClickListener() {
            @Override
            public void onClick(View v) {
                hideSoftInput();
                mInputText.clearFocus();
                if (v.getId() == R.id.increment) {
                    changeValueByOne(true);
                } else {
                    changeValueByOne(false);
                }
            }
        };
        OnLongClickListener onLongClickListener = new OnLongClickListener() {
            @Override
            public boolean onLongClick(View v) {
                hideSoftInput();
                mInputText.clearFocus();
                if (v.getId() == R.id.increment) {
                    postChangeCurrentByOneFromLongPress(true, 0);
                } else {
                    postChangeCurrentByOneFromLongPress(false, 0);
                }
                return true;
            }
        };
        if (!mHasSelectorWheel) {
            mIncrementButton = (ImageButton) findViewById(R.id.increment);
            if (mIncrementButton != null) {
                mIncrementButton.setOnClickListener(onClickListener);
                mIncrementButton.setOnLongClickListener(onLongClickListener);
            }
        } else {
            mIncrementButton = null;
        }
        if (!mHasSelectorWheel) {
            mDecrementButton = (ImageButton) findViewById(R.id.decrement);
            if (mDecrementButton != null) {
                mDecrementButton.setOnClickListener(onClickListener);
                mDecrementButton.setOnLongClickListener(onLongClickListener);
            }
        } else {
            mDecrementButton = null;
        }
        mInputText = (NumberPickerEditText) findViewById(R.id.numberpicker_input);
        mInputText.setOnFocusChangeListener(new OnFocusChangeListener() {
            @Override
            public void onFocusChange(View v, boolean hasFocus) {
                if (hasFocus) {
                    mInputText.selectAll();
                } else {
                    mInputText.setSelection(0, 0);
                    validateInputTextView(mInputText);
                }
            }
        });
        mInputText.setFilters(new InputFilter[] {
                new InputTextFilter()
        });
        mInputText.setRawInputType(InputType.TYPE_CLASS_NUMBER);
        mInputText.setImeOptions(EditorInfo.IME_ACTION_DONE);
        ViewConfiguration configuration = ViewConfiguration.get(context);
        mTouchSlop = configuration.getScaledTouchSlop();
        mMinimumFlingVelocity = configuration.getScaledMinimumFlingVelocity();
        mMaximumFlingVelocity = configuration.getScaledMaximumFlingVelocity()
                / NumberPicker.SELECTOR_MAX_FLING_VELOCITY_ADJUSTMENT;
        mTextSize = (int) mInputText.getTextSize();
        Paint paint = new Paint();
        paint.setAntiAlias(true);
        paint.setTextAlign(Align.CENTER);
        paint.setTextSize(mTextSize);
        paint.setTypeface(mInputText.getTypeface());
        ColorStateList colors = mInputText.getTextColors();
        int color = colors
                .getColorForState(View.ENABLED_STATE_SET, Color.WHITE);
        paint.setColor(color);
        mSelectorWheelPaint = paint;
        mFlingScroller = new Scroller(getContext(), null, true);
        mAdjustScroller = new Scroller(getContext(),
                new DecelerateInterpolator(2.5f));
        updateInputTextView();
        if (VERSION.SDK_INT >= 16
                && getImportantForAccessibility() == View.IMPORTANT_FOR_ACCESSIBILITY_AUTO) {
            setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_YES);
        }
    }

    @Override
    public void addFocusables(ArrayList<View> views, int direction,
            int focusableMode) {
        if ((focusableMode & NumberPicker.FOCUSABLES_ACCESSIBILITY) == NumberPicker.FOCUSABLES_ACCESSIBILITY) {
            views.add(this);
            return;
        }
        super.addFocusables(views, direction, focusableMode);
    }

    private void changeValueByOne(boolean increment) {
        if (mHasSelectorWheel) {
            mInputText.setVisibility(View.INVISIBLE);
            if (!moveToFinalScrollerPosition(mFlingScroller)) {
                moveToFinalScrollerPosition(mAdjustScroller);
            }
            mPreviousScrollerY = 0;
            if (increment) {
                mFlingScroller.startScroll(0, 0, 0, -mSelectorElementHeight,
                        NumberPicker.SNAP_SCROLL_DURATION);
            } else {
                mFlingScroller.startScroll(0, 0, 0, mSelectorElementHeight,
                        NumberPicker.SNAP_SCROLL_DURATION);
            }
            invalidate();
        } else {
            if (increment) {
                setValueInternal(mValue + 1, true);
            } else {
                setValueInternal(mValue - 1, true);
            }
        }
    }

    @Override
    public void computeScroll() {
        Scroller scroller = mFlingScroller;
        if (scroller.isFinished()) {
            scroller = mAdjustScroller;
            if (scroller.isFinished()) {
                return;
            }
        }
        scroller.computeScrollOffset();
        int currentScrollerY = scroller.getCurrY();
        if (mPreviousScrollerY == 0) {
            mPreviousScrollerY = scroller.getStartY();
        }
        scrollBy(0, currentScrollerY - mPreviousScrollerY);
        mPreviousScrollerY = currentScrollerY;
        if (scroller.isFinished()) {
            onScrollerFinished(scroller);
        } else {
            invalidate();
        }
    }

    private void decrementSelectorIndices(int[] selectorIndices) {
        for (int i = selectorIndices.length - 1; i > 0; i--) {
            selectorIndices[i] = selectorIndices[i - 1];
        }
        int nextScrollSelectorIndex = selectorIndices[1] - 1;
        if (mWrapSelectorWheel && nextScrollSelectorIndex < mMinValue) {
            nextScrollSelectorIndex = mMaxValue;
        }
        selectorIndices[0] = nextScrollSelectorIndex;
        ensureCachedScrollSelectorValue(nextScrollSelectorIndex);
    }

    @Override
    @SuppressLint("NewApi")
    protected boolean dispatchHoverEvent(MotionEvent event) {
        if (!mHasSelectorWheel) {
            return super.dispatchHoverEvent(event);
        }
        if (((AccessibilityManager) getContext().getSystemService(
                Context.ACCESSIBILITY_SERVICE)).isEnabled()) {
            switch (event.getAction() & MotionEvent.ACTION_MASK) {
                case MotionEvent.ACTION_HOVER_ENTER: {
                    sendAccessibilityEvent(AccessibilityEvent.TYPE_VIEW_HOVER_ENTER);
                    if (VERSION.SDK_INT >= 16) {
                        performAccessibilityAction(
                                AccessibilityNodeInfo.ACTION_ACCESSIBILITY_FOCUS,
                                null);
                    }
                }
                    break;
                case MotionEvent.ACTION_HOVER_MOVE: {
                    sendAccessibilityEvent(AccessibilityEvent.TYPE_VIEW_HOVER_EXIT);
                    sendAccessibilityEvent(AccessibilityEvent.TYPE_VIEW_HOVER_ENTER);
                    if (VERSION.SDK_INT >= 16) {
                        performAccessibilityAction(
                                AccessibilityNodeInfo.ACTION_ACCESSIBILITY_FOCUS,
                                null);
                    }
                }
                    break;
                case MotionEvent.ACTION_HOVER_EXIT: {
                    sendAccessibilityEvent(AccessibilityEvent.TYPE_VIEW_HOVER_EXIT);
                }
                    break;
            }
        }
        return false;
    }

    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        final int keyCode = event.getKeyCode();
        switch (keyCode) {
            case KeyEvent.KEYCODE_DPAD_CENTER:
            case KeyEvent.KEYCODE_ENTER:
                removeAllCallbacks();
                break;
        }
        return super.dispatchKeyEvent(event);
    }

    @Override
    public boolean dispatchTouchEvent(MotionEvent event) {
        switch (event.getAction() & MotionEvent.ACTION_MASK) {
            case MotionEvent.ACTION_CANCEL:
            case MotionEvent.ACTION_UP:
                removeAllCallbacks();
                break;
        }
        return super.dispatchTouchEvent(event);
    }

    @Override
    public boolean dispatchTrackballEvent(MotionEvent event) {
        switch (event.getAction() & MotionEvent.ACTION_MASK) {
            case MotionEvent.ACTION_CANCEL:
            case MotionEvent.ACTION_UP:
                removeAllCallbacks();
                break;
        }
        return super.dispatchTrackballEvent(event);
    }

    private void ensureCachedScrollSelectorValue(int selectorIndex) {
        SparseArray<String> cache = mSelectorIndexToStringCache;
        String scrollSelectorValue = cache.get(selectorIndex);
        if (scrollSelectorValue != null) {
            return;
        }
        if (selectorIndex < mMinValue || selectorIndex > mMaxValue) {
            scrollSelectorValue = "";
        } else {
            if (mDisplayedValues != null) {
                int displayedValueIndex = selectorIndex - mMinValue;
                scrollSelectorValue = mDisplayedValues[displayedValueIndex];
            } else {
                scrollSelectorValue = formatNumber(selectorIndex);
            }
        }
        cache.put(selectorIndex, scrollSelectorValue);
    }

    private boolean ensureScrollWheelAdjusted() {
        int deltaY = mInitialScrollOffset - mCurrentScrollOffset;
        if (deltaY != 0) {
            mPreviousScrollerY = 0;
            if (Math.abs(deltaY) > mSelectorElementHeight / 2) {
                deltaY += deltaY > 0 ? -mSelectorElementHeight
                        : mSelectorElementHeight;
            }
            mAdjustScroller.startScroll(0, 0, 0, deltaY,
                    NumberPicker.SELECTOR_ADJUSTMENT_DURATION_MILLIS);
            invalidate();
            return true;
        }
        return false;
    }

    private void fling(int velocityY) {
        mPreviousScrollerY = 0;
        if (velocityY > 0) {
            mFlingScroller
                    .fling(0, 0, 0, velocityY, 0, 0, 0, Integer.MAX_VALUE);
        } else {
            mFlingScroller.fling(0, Integer.MAX_VALUE, 0, velocityY, 0, 0, 0,
                    Integer.MAX_VALUE);
        }
        invalidate();
    }

    private String formatNumber(int value) {
        return mFormatter != null ? mFormatter.format(value) : String
                .valueOf(value);
    }

    @Override
    protected float getBottomFadingEdgeStrength() {
        return NumberPicker.TOP_AND_BOTTOM_FADING_EDGE_STRENGTH;
    }

    public String[] getDisplayedValues() {
        return mDisplayedValues;
    }

    public NumberPickerEditText getInputField() {
        return mInputText;
    }

    public int getMaxValue() {
        return mMaxValue;
    }

    public int getMinValue() {
        return mMinValue;
    }

    private int getSelectedPos(String value) {
        if (mDisplayedValues != null) {
            for (int i = 0; i < mDisplayedValues.length; i++) {
                value = value.toLowerCase();
                if (mDisplayedValues[i].toLowerCase().startsWith(value)) {
                    return mMinValue + i;
                }
            }
        }
        try {
            return Integer.parseInt(value);
        } catch (NumberFormatException e) {
        }
        return mMinValue;
    }

    @Override
    public int getSolidColor() {
        return mSolidColor;
    }

    @Override
    protected float getTopFadingEdgeStrength() {
        return NumberPicker.TOP_AND_BOTTOM_FADING_EDGE_STRENGTH;
    }

    public int getValue() {
        return mValue;
    }

    private int getWrappedSelectorIndex(int selectorIndex) {
        if (selectorIndex > mMaxValue) {
            return mMinValue + (selectorIndex - mMaxValue)
                    % (mMaxValue - mMinValue) - 1;
        } else if (selectorIndex < mMinValue) {
            return mMaxValue - (mMinValue - selectorIndex)
                    % (mMaxValue - mMinValue) + 1;
        }
        return selectorIndex;
    }

    public boolean getWrapSelectorWheel() {
        return mWrapSelectorWheel;
    }

    private void hideSoftInput() {
        InputMethodManager inputMethodManager = (InputMethodManager) getContext()
                .getSystemService(Context.INPUT_METHOD_SERVICE);
        if (inputMethodManager != null
                && inputMethodManager.isActive(mInputText)) {
            inputMethodManager.hideSoftInputFromWindow(getWindowToken(), 0);
            if (mHasSelectorWheel) {
                mInputText.setVisibility(View.INVISIBLE);
            }
        }
    }

    private void incrementSelectorIndices(int[] selectorIndices) {
        for (int i = 0; i < selectorIndices.length - 1; i++) {
            selectorIndices[i] = selectorIndices[i + 1];
        }
        int nextScrollSelectorIndex = selectorIndices[selectorIndices.length - 2] + 1;
        if (mWrapSelectorWheel && nextScrollSelectorIndex > mMaxValue) {
            nextScrollSelectorIndex = mMinValue;
        }
        selectorIndices[selectorIndices.length - 1] = nextScrollSelectorIndex;
        ensureCachedScrollSelectorValue(nextScrollSelectorIndex);
    }

    private void initializeFadingEdges() {
        setVerticalFadingEdgeEnabled(true);
        setFadingEdgeLength((getBottom() - getTop() - mTextSize) / 2);
    }

    private void initializeSelectorWheel() {
        initializeSelectorWheelIndices();
        int[] selectorIndices = mSelectorIndices;
        int totalTextHeight = selectorIndices.length * mTextSize;
        float totalTextGapHeight = getBottom() - getTop() - totalTextHeight;
        float textGapCount = selectorIndices.length;
        mSelectorTextGapHeight = (int) (totalTextGapHeight / textGapCount + 0.5f);
        mSelectorElementHeight = mTextSize + mSelectorTextGapHeight;
        int editTextTextPosition = mInputText.getBaseline()
                + mInputText.getTop();
        mInitialScrollOffset = editTextTextPosition - mSelectorElementHeight
                * NumberPicker.SELECTOR_WHELL_MIDDLE_ITEM_INDEX;
        mCurrentScrollOffset = mInitialScrollOffset;
        updateInputTextView();
    }

    private void initializeSelectorWheelIndices() {
        mSelectorIndexToStringCache.clear();
        int[] selectorIndices = mSelectorIndices;
        int current = getValue();
        for (int i = 0; i < mSelectorIndices.length; i++) {
            int selectorIndex = current + i
                    - NumberPicker.SELECTOR_WHELL_MIDDLE_ITEM_INDEX;
            if (mWrapSelectorWheel) {
                selectorIndex = getWrappedSelectorIndex(selectorIndex);
            }
            selectorIndices[i] = selectorIndex;
            ensureCachedScrollSelectorValue(selectorIndices[i]);
        }
    }

    private int makeMeasureSpec(int measureSpec, int maxSize) {
        if (maxSize == NumberPicker.SIZE_UNSPECIFIED) {
            return measureSpec;
        }
        final int size = MeasureSpec.getSize(measureSpec);
        final int mode = MeasureSpec.getMode(measureSpec);
        switch (mode) {
            case MeasureSpec.EXACTLY:
                return measureSpec;
            case MeasureSpec.AT_MOST:
                return MeasureSpec.makeMeasureSpec(Math.min(size, maxSize),
                        MeasureSpec.EXACTLY);
            case MeasureSpec.UNSPECIFIED:
                return MeasureSpec.makeMeasureSpec(maxSize, MeasureSpec.EXACTLY);
            default:
                throw new IllegalArgumentException("Unknown measure mode: " + mode);
        }
    }

    private boolean moveToFinalScrollerPosition(Scroller scroller) {
        scroller.forceFinished(true);
        int amountToScroll = scroller.getFinalY() - scroller.getCurrY();
        int futureScrollOffset = (mCurrentScrollOffset + amountToScroll)
                % mSelectorElementHeight;
        int overshootAdjustment = mInitialScrollOffset - futureScrollOffset;
        if (overshootAdjustment != 0) {
            if (Math.abs(overshootAdjustment) > mSelectorElementHeight / 2) {
                if (overshootAdjustment > 0) {
                    overshootAdjustment -= mSelectorElementHeight;
                } else {
                    overshootAdjustment += mSelectorElementHeight;
                }
            }
            amountToScroll += overshootAdjustment;
            scrollBy(0, amountToScroll);
            return true;
        }
        return false;
    }

    private void notifyChange(int previous, int current) {
        if (mOnValueChangeListener != null) {
            mOnValueChangeListener.onValueChange(this, previous, mValue);
        }
    }

    @Override
    protected void onDetachedFromWindow() {
        removeAllCallbacks();
    }

    @Override
    protected void onDraw(Canvas canvas) {
        if (!mHasSelectorWheel) {
            super.onDraw(canvas);
            return;
        }
        float x = (getRight() - getLeft()) / 2;
        float y = mCurrentScrollOffset;
        if (mVirtualButtonPressedDrawable != null
                && mScrollState == OnScrollListener.SCROLL_STATE_IDLE) {
            if (mDecrementVirtualButtonPressed) {

                mVirtualButtonPressedDrawable
                        .setState(org.holoeverywhere.internal._View.PRESSED_STATE_SET);
                mVirtualButtonPressedDrawable.setBounds(0, 0, getRight(),
                        mTopSelectionDividerTop);
                mVirtualButtonPressedDrawable.draw(canvas);
            }
            if (mIncrementVirtualButtonPressed) {
                mVirtualButtonPressedDrawable
                        .setState(org.holoeverywhere.internal._View.PRESSED_STATE_SET);
                mVirtualButtonPressedDrawable.setBounds(0,
                        mBottomSelectionDividerBottom, getRight(), getBottom());
                mVirtualButtonPressedDrawable.draw(canvas);
            }
        }
        int[] selectorIndices = mSelectorIndices;
        for (int i = 0; i < selectorIndices.length; i++) {
            int selectorIndex = selectorIndices[i];
            String scrollSelectorValue = mSelectorIndexToStringCache
                    .get(selectorIndex);
            if (i != NumberPicker.SELECTOR_WHELL_MIDDLE_ITEM_INDEX
                    || mInputText.getVisibility() != View.VISIBLE) {
                canvas.drawText(scrollSelectorValue, x, y, mSelectorWheelPaint);
            }
            y += mSelectorElementHeight;
        }
        if (mSelectionDivider != null) {
            int topOfTopDivider = mTopSelectionDividerTop;
            int bottomOfTopDivider = topOfTopDivider + mSelectionDividerHeight;
            mSelectionDivider.setBounds(0, topOfTopDivider, getRight(),
                    bottomOfTopDivider);
            mSelectionDivider.draw(canvas);
            int bottomOfBottomDivider = mBottomSelectionDividerBottom;
            int topOfBottomDivider = bottomOfBottomDivider
                    - mSelectionDividerHeight;
            mSelectionDivider.setBounds(0, topOfBottomDivider, getRight(),
                    bottomOfBottomDivider);
            mSelectionDivider.draw(canvas);
        }
    }

    @Override
    @SuppressLint("NewApi")
    public void onInitializeAccessibilityEvent(AccessibilityEvent event) {
        super.onInitializeAccessibilityEvent(event);
        event.setClassName(NumberPicker.class.getName());
        event.setScrollable(true);
        event.setScrollY((mMinValue + mValue) * mSelectorElementHeight);
        event.setMaxScrollY((mMaxValue - mMinValue) * mSelectorElementHeight);
    }

    @Override
    public boolean onInterceptTouchEvent(MotionEvent event) {
        if (!mHasSelectorWheel || !isEnabled()) {
            return false;
        }
        switch (event.getAction() & MotionEvent.ACTION_MASK) {
            case MotionEvent.ACTION_DOWN: {
                removeAllCallbacks();
                mInputText.setVisibility(View.INVISIBLE);
                mLastDownOrMoveEventY = mLastDownEventY = event.getY();
                mLastDownEventTime = event.getEventTime();
                mIngonreMoveEvents = false;
                mShowSoftInputOnTap = false;
                if (mLastDownEventY < mTopSelectionDividerTop) {
                    if (mScrollState == OnScrollListener.SCROLL_STATE_IDLE) {
                        mPressedStateHelper
                                .buttonPressDelayed(PressedStateHelper.BUTTON_DECREMENT);
                    }
                } else if (mLastDownEventY > mBottomSelectionDividerBottom) {
                    if (mScrollState == OnScrollListener.SCROLL_STATE_IDLE) {
                        mPressedStateHelper
                                .buttonPressDelayed(PressedStateHelper.BUTTON_INCREMENT);
                    }
                }
                getParent().requestDisallowInterceptTouchEvent(true);
                if (!mFlingScroller.isFinished()) {
                    mFlingScroller.forceFinished(true);
                    mAdjustScroller.forceFinished(true);
                    onScrollStateChange(OnScrollListener.SCROLL_STATE_IDLE);
                } else if (!mAdjustScroller.isFinished()) {
                    mFlingScroller.forceFinished(true);
                    mAdjustScroller.forceFinished(true);
                } else if (mLastDownEventY < mTopSelectionDividerTop) {
                    hideSoftInput();
                    postChangeCurrentByOneFromLongPress(false,
                            ViewConfiguration.getLongPressTimeout());
                } else if (mLastDownEventY > mBottomSelectionDividerBottom) {
                    hideSoftInput();
                    postChangeCurrentByOneFromLongPress(true,
                            ViewConfiguration.getLongPressTimeout());
                } else {
                    mShowSoftInputOnTap = true;
                    postBeginSoftInputOnLongPressCommand();
                }
                return true;
            }
        }
        return false;
    }

    @Override
    protected void onLayout(boolean changed, int left, int top, int right,
            int bottom) {
        if (!mHasSelectorWheel) {
            super.onLayout(changed, left, top, right, bottom);
            return;
        }
        final int msrdWdth = getMeasuredWidth();
        final int msrdHght = getMeasuredHeight();
        final int inptTxtMsrdWdth = mInputText.getMeasuredWidth();
        final int inptTxtMsrdHght = mInputText.getMeasuredHeight();
        final int inptTxtLeft = (msrdWdth - inptTxtMsrdWdth) / 2;
        final int inptTxtTop = (msrdHght - inptTxtMsrdHght) / 2;
        final int inptTxtRight = inptTxtLeft + inptTxtMsrdWdth;
        final int inptTxtBottom = inptTxtTop + inptTxtMsrdHght;
        mInputText.layout(inptTxtLeft, inptTxtTop, inptTxtRight, inptTxtBottom);
        if (changed) {
            initializeSelectorWheel();
            initializeFadingEdges();
            mTopSelectionDividerTop = (getHeight() - mSelectionDividersDistance)
                    / 2 - mSelectionDividerHeight;
            mBottomSelectionDividerBottom = mTopSelectionDividerTop + 2
                    * mSelectionDividerHeight + mSelectionDividersDistance;
        }
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        if (!mHasSelectorWheel) {
            super.onMeasure(widthMeasureSpec, heightMeasureSpec);
            return;
        }
        final int newWidthMeasureSpec = makeMeasureSpec(widthMeasureSpec,
                mMaxWidth);
        final int newHeightMeasureSpec = makeMeasureSpec(heightMeasureSpec,
                mMaxHeight);
        super.onMeasure(newWidthMeasureSpec, newHeightMeasureSpec);
        final int widthSize = resolveSizeAndStateRespectingMinSize(mMinWidth,
                getMeasuredWidth(), widthMeasureSpec);
        final int heightSize = resolveSizeAndStateRespectingMinSize(mMinHeight,
                getMeasuredHeight(), heightMeasureSpec);
        setMeasuredDimension(widthSize, heightSize);
    }

    private void onScrollerFinished(Scroller scroller) {
        if (scroller == mFlingScroller) {
            if (!ensureScrollWheelAdjusted()) {
                updateInputTextView();
            }
            onScrollStateChange(OnScrollListener.SCROLL_STATE_IDLE);
        } else {
            if (mScrollState != OnScrollListener.SCROLL_STATE_TOUCH_SCROLL) {
                updateInputTextView();
            }
        }
    }

    private void onScrollStateChange(int scrollState) {
        if (mScrollState == scrollState) {
            return;
        }
        mScrollState = scrollState;
        if (mOnScrollListener != null) {
            mOnScrollListener.onScrollStateChange(this, scrollState);
        }
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        if (!isEnabled() || !mHasSelectorWheel) {
            return false;
        }
        if (mVelocityTracker == null) {
            mVelocityTracker = VelocityTracker.obtain();
        }
        mVelocityTracker.addMovement(event);
        switch (event.getAction() & MotionEvent.ACTION_MASK) {
            case MotionEvent.ACTION_MOVE: {
                if (mIngonreMoveEvents) {
                    break;
                }
                float currentMoveY = event.getY();
                if (mScrollState != OnScrollListener.SCROLL_STATE_TOUCH_SCROLL) {
                    int deltaDownY = (int) Math.abs(currentMoveY - mLastDownEventY);
                    if (deltaDownY > mTouchSlop) {
                        removeAllCallbacks();
                        onScrollStateChange(OnScrollListener.SCROLL_STATE_TOUCH_SCROLL);
                    }
                } else {
                    int deltaMoveY = (int) (currentMoveY - mLastDownOrMoveEventY);
                    scrollBy(0, deltaMoveY);
                    invalidate();
                }
                mLastDownOrMoveEventY = currentMoveY;
            }
                break;
            case MotionEvent.ACTION_UP: {
                removeBeginSoftInputCommand();
                removeChangeCurrentByOneFromLongPress();
                mPressedStateHelper.cancel();
                VelocityTracker velocityTracker = mVelocityTracker;
                velocityTracker.computeCurrentVelocity(1000, mMaximumFlingVelocity);
                int initialVelocity = (int) velocityTracker.getYVelocity();
                if (Math.abs(initialVelocity) > mMinimumFlingVelocity) {
                    fling(initialVelocity);
                    onScrollStateChange(OnScrollListener.SCROLL_STATE_FLING);
                } else {
                    int eventY = (int) event.getY();
                    int deltaMoveY = (int) Math.abs(eventY - mLastDownEventY);
                    long deltaTime = event.getEventTime() - mLastDownEventTime;
                    if (deltaMoveY <= mTouchSlop
                            && deltaTime < ViewConfiguration.getTapTimeout()) {
                        if (mShowSoftInputOnTap) {
                            mShowSoftInputOnTap = false;
                            showSoftInput();
                        } else {
                            int selectorIndexOffset = eventY
                                    / mSelectorElementHeight
                                    - NumberPicker.SELECTOR_WHELL_MIDDLE_ITEM_INDEX;
                            if (selectorIndexOffset > 0) {
                                changeValueByOne(true);
                                mPressedStateHelper
                                        .buttonTapped(PressedStateHelper.BUTTON_INCREMENT);
                            } else if (selectorIndexOffset < 0) {
                                changeValueByOne(false);
                                mPressedStateHelper
                                        .buttonTapped(PressedStateHelper.BUTTON_DECREMENT);
                            }
                        }
                    } else {
                        ensureScrollWheelAdjusted();
                    }
                    onScrollStateChange(OnScrollListener.SCROLL_STATE_IDLE);
                }
                mVelocityTracker.recycle();
                mVelocityTracker = null;
            }
                break;
        }
        return true;
    }

    private void postBeginSoftInputOnLongPressCommand() {
        if (mBeginSoftInputOnLongPressCommand == null) {
            mBeginSoftInputOnLongPressCommand = new BeginSoftInputOnLongPressCommand();
        } else {
            removeCallbacks(mBeginSoftInputOnLongPressCommand);
        }
        postDelayed(mBeginSoftInputOnLongPressCommand,
                ViewConfiguration.getLongPressTimeout());
    }

    private void postChangeCurrentByOneFromLongPress(boolean increment,
            long delayMillis) {
        if (mChangeCurrentByOneFromLongPressCommand == null) {
            mChangeCurrentByOneFromLongPressCommand = new ChangeCurrentByOneFromLongPressCommand();
        } else {
            removeCallbacks(mChangeCurrentByOneFromLongPressCommand);
        }
        mChangeCurrentByOneFromLongPressCommand.setStep(increment);
        postDelayed(mChangeCurrentByOneFromLongPressCommand, delayMillis);
    }

    private void postSetSelectionCommand(int selectionStart, int selectionEnd) {
        if (mSetSelectionCommand == null) {
            mSetSelectionCommand = new SetSelectionCommand();
        } else {
            removeCallbacks(mSetSelectionCommand);
        }
        mSetSelectionCommand.mSelectionStart = selectionStart;
        mSetSelectionCommand.mSelectionEnd = selectionEnd;
        post(mSetSelectionCommand);
    }

    private void removeAllCallbacks() {
        if (mChangeCurrentByOneFromLongPressCommand != null) {
            removeCallbacks(mChangeCurrentByOneFromLongPressCommand);
        }
        if (mSetSelectionCommand != null) {
            removeCallbacks(mSetSelectionCommand);
        }
        if (mBeginSoftInputOnLongPressCommand != null) {
            removeCallbacks(mBeginSoftInputOnLongPressCommand);
        }
        mPressedStateHelper.cancel();
    }

    private void removeBeginSoftInputCommand() {
        if (mBeginSoftInputOnLongPressCommand != null) {
            removeCallbacks(mBeginSoftInputOnLongPressCommand);
        }
    }

    private void removeChangeCurrentByOneFromLongPress() {
        if (mChangeCurrentByOneFromLongPressCommand != null) {
            removeCallbacks(mChangeCurrentByOneFromLongPressCommand);
        }
    }

    private int resolveSizeAndStateRespectingMinSize(int minSize,
            int measuredSize, int measureSpec) {
        if (minSize != NumberPicker.SIZE_UNSPECIFIED) {
            final int desiredWidth = Math.max(minSize, measuredSize);
            return org.holoeverywhere.internal._View
                    .supportResolveSizeAndState(desiredWidth, measureSpec, 0);
        } else {
            return measuredSize;
        }
    }

    @Override
    public void scrollBy(int x, int y) {
        int[] selectorIndices = mSelectorIndices;
        if (!mWrapSelectorWheel
                && y > 0
                && selectorIndices[NumberPicker.SELECTOR_WHELL_MIDDLE_ITEM_INDEX] <= mMinValue) {
            mCurrentScrollOffset = mInitialScrollOffset;
            return;
        }
        if (!mWrapSelectorWheel
                && y < 0
                && selectorIndices[NumberPicker.SELECTOR_WHELL_MIDDLE_ITEM_INDEX] >= mMaxValue) {
            mCurrentScrollOffset = mInitialScrollOffset;
            return;
        }
        mCurrentScrollOffset += y;
        while (mCurrentScrollOffset - mInitialScrollOffset > mSelectorTextGapHeight) {
            mCurrentScrollOffset -= mSelectorElementHeight;
            decrementSelectorIndices(selectorIndices);
            setValueInternal(
                    selectorIndices[NumberPicker.SELECTOR_WHELL_MIDDLE_ITEM_INDEX],
                    true);
            if (!mWrapSelectorWheel
                    && selectorIndices[NumberPicker.SELECTOR_WHELL_MIDDLE_ITEM_INDEX] <= mMinValue) {
                mCurrentScrollOffset = mInitialScrollOffset;
            }
        }
        while (mCurrentScrollOffset - mInitialScrollOffset < -mSelectorTextGapHeight) {
            mCurrentScrollOffset += mSelectorElementHeight;
            incrementSelectorIndices(selectorIndices);
            setValueInternal(
                    selectorIndices[NumberPicker.SELECTOR_WHELL_MIDDLE_ITEM_INDEX],
                    true);
            if (!mWrapSelectorWheel
                    && selectorIndices[NumberPicker.SELECTOR_WHELL_MIDDLE_ITEM_INDEX] >= mMaxValue) {
                mCurrentScrollOffset = mInitialScrollOffset;
            }
        }
    }

    public void setDisplayedValues(String[] displayedValues) {
        if (mDisplayedValues == displayedValues) {
            return;
        }
        mDisplayedValues = displayedValues;
        if (mDisplayedValues != null) {
            mInputText.setRawInputType(InputType.TYPE_CLASS_TEXT
                    | InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS);
        } else {
            mInputText.setRawInputType(InputType.TYPE_CLASS_NUMBER);
        }
        updateInputTextView();
        initializeSelectorWheelIndices();
        tryComputeMaxWidth();
    }

    @Override
    public void setEnabled(boolean enabled) {
        super.setEnabled(enabled);
        if (!mHasSelectorWheel) {
            mIncrementButton.setEnabled(enabled);
        }
        if (!mHasSelectorWheel) {
            mDecrementButton.setEnabled(enabled);
        }
        mInputText.setEnabled(enabled);
    }

    public void setFormatter(Formatter formatter) {
        if (formatter == mFormatter) {
            return;
        }
        mFormatter = formatter;
        initializeSelectorWheelIndices();
        updateInputTextView();
    }

    public void setMaxValue(int maxValue) {
        if (mMaxValue == maxValue) {
            return;
        }
        if (maxValue < 0) {
            throw new IllegalArgumentException("maxValue must be >= 0");
        }
        mMaxValue = maxValue;
        if (mMaxValue < mValue) {
            mValue = mMaxValue;
        }
        boolean wrapSelectorWheel = mMaxValue - mMinValue > mSelectorIndices.length;
        setWrapSelectorWheel(wrapSelectorWheel);
        initializeSelectorWheelIndices();
        updateInputTextView();
        tryComputeMaxWidth();
        invalidate();
    }

    public void setMinValue(int minValue) {
        if (mMinValue == minValue) {
            return;
        }
        if (minValue < 0) {
            throw new IllegalArgumentException("minValue must be >= 0");
        }
        mMinValue = minValue;
        if (mMinValue > mValue) {
            mValue = mMinValue;
        }
        boolean wrapSelectorWheel = mMaxValue - mMinValue > mSelectorIndices.length;
        setWrapSelectorWheel(wrapSelectorWheel);
        initializeSelectorWheelIndices();
        updateInputTextView();
        tryComputeMaxWidth();
        invalidate();
    }

    public void setOnLongPressUpdateInterval(long intervalMillis) {
        mLongPressUpdateInterval = intervalMillis;
    }

    public void setOnScrollListener(OnScrollListener onScrollListener) {
        mOnScrollListener = onScrollListener;
    }

    public void setOnValueChangedListener(
            OnValueChangeListener onValueChangedListener) {
        mOnValueChangeListener = onValueChangedListener;
    }

    public void setValue(int value) {
        setValueInternal(value, false);
    }

    private void setValueInternal(int current, boolean notifyChange) {
        if (mValue == current) {
            return;
        }
        if (mWrapSelectorWheel) {
            current = getWrappedSelectorIndex(current);
        } else {
            current = Math.max(current, mMinValue);
            current = Math.min(current, mMaxValue);
        }
        int previous = mValue;
        mValue = current;
        updateInputTextView();
        if (notifyChange) {
            notifyChange(previous, current);
        }
        initializeSelectorWheelIndices();
        invalidate();
    }

    public void setWrapSelectorWheel(boolean wrapSelectorWheel) {
        final boolean wrappingAllowed = mMaxValue - mMinValue >= mSelectorIndices.length;
        if ((!wrapSelectorWheel || wrappingAllowed)
                && wrapSelectorWheel != mWrapSelectorWheel) {
            mWrapSelectorWheel = wrapSelectorWheel;
        }
    }

    private void showSoftInput() {
        InputMethodManager inputMethodManager = (InputMethodManager) getContext()
                .getSystemService(Context.INPUT_METHOD_SERVICE);
        if (inputMethodManager != null) {
            if (mHasSelectorWheel) {
                mInputText.setVisibility(View.VISIBLE);
            }
            mInputText.requestFocus();
            inputMethodManager.showSoftInput(mInputText, 0);
        }
    }

    private void tryComputeMaxWidth() {
        if (!mComputeMaxWidth) {
            return;
        }
        int maxTextWidth = 0;
        if (mDisplayedValues == null) {
            float maxDigitWidth = 0;
            for (int i = 0; i <= 9; i++) {
                final float digitWidth = mSelectorWheelPaint.measureText(String
                        .valueOf(i));
                if (digitWidth > maxDigitWidth) {
                    maxDigitWidth = digitWidth;
                }
            }
            int numberOfDigits = 0;
            int current = mMaxValue;
            while (current > 0) {
                numberOfDigits++;
                current = current / 10;
            }
            maxTextWidth = (int) (numberOfDigits * maxDigitWidth);
        } else {
            final int valueCount = mDisplayedValues.length;
            for (int i = 0; i < valueCount; i++) {
                final float textWidth = mSelectorWheelPaint
                        .measureText(mDisplayedValues[i]);
                if (textWidth > maxTextWidth) {
                    maxTextWidth = (int) textWidth;
                }
            }
        }
        maxTextWidth += mInputText.getPaddingLeft()
                + mInputText.getPaddingRight();
        if (mMaxWidth != maxTextWidth) {
            if (maxTextWidth > mMinWidth) {
                mMaxWidth = maxTextWidth;
            } else {
                mMaxWidth = mMinWidth;
            }
            invalidate();
        }
    }

    private boolean updateInputTextView() {
        String text = mDisplayedValues == null ? formatNumber(mValue)
                : mDisplayedValues[mValue - mMinValue];
        if (!TextUtils.isEmpty(text)
                && !text.equals(mInputText.getText().toString())) {
            mInputText.setText(text);
            return true;
        }
        return false;
    }

    private void validateInputTextView(NumberPickerEditText v) {
        String str = String.valueOf(v.getText());
        if (TextUtils.isEmpty(str)) {
            updateInputTextView();
        } else {
            int current = getSelectedPos(str.toString());
            setValueInternal(current, true);
        }
    }
}
