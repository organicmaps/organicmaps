
package org.holoeverywhere.widget;

import org.holoeverywhere.R;
import org.holoeverywhere.text.AllCapsTransformationMethod;
import org.holoeverywhere.text.TransformationMethod;

import android.content.Context;
import android.content.res.ColorStateList;
import android.content.res.Resources;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.Typeface;
import android.graphics.drawable.Drawable;
import android.support.v4.view.MotionEventCompat;
import android.text.Layout;
import android.text.StaticLayout;
import android.text.TextPaint;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.VelocityTracker;
import android.view.ViewConfiguration;
import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityNodeInfo;
import android.widget.CompoundButton;

public class Switch extends CompoundButton {
    private static final int[] CHECKED_STATE_SET = {
            android.R.attr.state_checked
    };
    private static final int MONOSPACE = 3;
    private static final int SANS = 1;
    private static final int SERIF = 2;
    private static final int TOUCH_MODE_DOWN = 1;
    private static final int TOUCH_MODE_DRAGGING = 2;
    private static final int TOUCH_MODE_IDLE = 0;
    private int mMinFlingVelocity;
    private Layout mOffLayout;
    private Layout mOnLayout;
    private int mSwitchBottom;
    private int mSwitchHeight;
    private int mSwitchLeft;
    private int mSwitchMinWidth;
    private int mSwitchPadding;
    private int mSwitchRight;
    private int mSwitchTop;
    private TransformationMethod mSwitchTransformationMethod;
    private int mSwitchWidth;
    private final Rect mTempRect = new Rect();
    private ColorStateList mTextColors;
    private CharSequence mTextOff;
    private CharSequence mTextOn;
    private TextPaint mTextPaint;
    private Drawable mThumbDrawable;
    private float mThumbPosition;
    private int mThumbTextPadding;
    private int mThumbWidth;
    private boolean mToggleWhenClick;
    private int mTouchMode;
    private int mTouchSlop;
    private float mTouchX;
    private float mTouchY;
    private Drawable mTrackDrawable;
    private VelocityTracker mVelocityTracker = VelocityTracker.obtain();

    public Switch(Context context) {
        this(context, null);
    }

    public Switch(Context context, AttributeSet attrs) {
        this(context, attrs, R.attr.switchStyle);
    }

    public Switch(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        mTextPaint = new TextPaint(Paint.ANTI_ALIAS_FLAG);
        Resources res = getResources();
        mTextPaint.density = res.getDisplayMetrics().density;
        // TODO
        // mTextPaint.setCompatibilityScaling(res.getCompatibilityInfo().applicationScale);
        TypedArray a = context.obtainStyledAttributes(attrs,
                R.styleable.Switch, defStyle, 0);
        mThumbDrawable = a.getDrawable(R.styleable.Switch_thumb);
        mTrackDrawable = a.getDrawable(R.styleable.Switch_track);
        mTextOn = a.getText(R.styleable.Switch_textOn);
        mTextOff = a.getText(R.styleable.Switch_textOff);
        mThumbTextPadding = a.getDimensionPixelSize(
                R.styleable.Switch_thumbTextPadding, 0);
        mSwitchMinWidth = a.getDimensionPixelSize(
                R.styleable.Switch_switchMinWidth, 0);
        mSwitchPadding = a.getDimensionPixelSize(
                R.styleable.Switch_switchPadding, 0);
        mToggleWhenClick = a.getBoolean(R.styleable.Switch_toggleWhenClick, true);
        int appearance = a.getResourceId(R.styleable.Switch_switchTextAppearance, 0);
        if (appearance != 0) {
            setSwitchTextAppearance(context, appearance);
        }
        a.recycle();
        ViewConfiguration config = ViewConfiguration.get(context);
        mTouchSlop = config.getScaledTouchSlop();
        mMinFlingVelocity = config.getScaledMinimumFlingVelocity();
        refreshDrawableState();
        setChecked(isChecked());
    }

    private void animateThumbToCheckedState(boolean newCheckedState) {
        // TODO animate!
        // float targetPos = newCheckedState ? 0 : getThumbScrollRange();
        // mThumbPosition = targetPos;
        setChecked(newCheckedState);
    }

    private void cancelSuperTouch(MotionEvent ev) {
        MotionEvent cancel = MotionEvent.obtain(ev);
        cancel.setAction(MotionEvent.ACTION_CANCEL);
        super.onTouchEvent(cancel);
        cancel.recycle();
    }

    @Override
    protected void drawableStateChanged() {
        super.drawableStateChanged();
        int[] myDrawableState = getDrawableState();
        if (mThumbDrawable != null) {
            mThumbDrawable.setState(myDrawableState);
        }
        if (mTrackDrawable != null) {
            mTrackDrawable.setState(myDrawableState);
        }
        invalidate();
    }

    @Override
    public int getCompoundPaddingRight() {
        int padding = super.getCompoundPaddingRight() + mSwitchWidth;
        if (!TextUtils.isEmpty(getText())) {
            padding += mSwitchPadding;
        }
        return padding;
    }

    public int getSwitchMinWidth() {
        return mSwitchMinWidth;
    }

    public int getSwitchPadding() {
        return mSwitchPadding;
    }

    private boolean getTargetCheckedState() {
        return mThumbPosition >= getThumbScrollRange() / 2;
    }

    public CharSequence getTextOff() {
        return mTextOff;
    }

    public CharSequence getTextOn() {
        return mTextOn;
    }

    public Drawable getThumbDrawable() {
        return mThumbDrawable;
    }

    private int getThumbScrollRange() {
        if (mTrackDrawable == null) {
            return 0;
        }
        mTrackDrawable.getPadding(mTempRect);
        return mSwitchWidth - mThumbWidth - mTempRect.left - mTempRect.right;
    }

    public int getThumbTextPadding() {
        return mThumbTextPadding;
    }

    public Drawable getTrackDrawable() {
        return mTrackDrawable;
    }

    private boolean hitThumb(float x, float y) {
        mThumbDrawable.getPadding(mTempRect);
        final int thumbTop = mSwitchTop - mTouchSlop;
        final int thumbLeft = mSwitchLeft + (int) (mThumbPosition + 0.5f) - mTouchSlop;
        final int thumbRight = thumbLeft + mThumbWidth +
                mTempRect.left + mTempRect.right + mTouchSlop;
        final int thumbBottom = mSwitchBottom + mTouchSlop;
        return x > thumbLeft && x < thumbRight && y > thumbTop && y < thumbBottom;
    }

    public boolean isToggleWhenClick() {
        return mToggleWhenClick;
    }

    @Override
    public void jumpDrawablesToCurrentState() {
        super.jumpDrawablesToCurrentState();
        mThumbDrawable.jumpToCurrentState();
        mTrackDrawable.jumpToCurrentState();
    }

    private Layout makeLayout(CharSequence text) {
        final CharSequence transformed = mSwitchTransformationMethod != null
                ? mSwitchTransformationMethod.getTransformation(text, this)
                : text;
        return new StaticLayout(transformed, mTextPaint,
                (int) Math.ceil(Layout.getDesiredWidth(transformed, mTextPaint)),
                Layout.Alignment.ALIGN_NORMAL, 1.f, 0, true);
    }

    @Override
    protected int[] onCreateDrawableState(int extraSpace) {
        final int[] drawableState = super.onCreateDrawableState(extraSpace + 1);
        if (isChecked()) {
            mergeDrawableStates(drawableState, CHECKED_STATE_SET);
        }
        return drawableState;
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        int switchLeft = mSwitchLeft;
        int switchTop = mSwitchTop;
        int switchRight = mSwitchRight;
        int switchBottom = mSwitchBottom;
        mTrackDrawable.setBounds(switchLeft, switchTop, switchRight, switchBottom);
        mTrackDrawable.draw(canvas);
        final int saveState = canvas.save();
        mTrackDrawable.getPadding(mTempRect);
        int switchInnerLeft = switchLeft + mTempRect.left;
        int switchInnerTop = switchTop + mTempRect.top;
        int switchInnerRight = switchRight - mTempRect.right;
        int switchInnerBottom = switchBottom - mTempRect.bottom;
        canvas.clipRect(switchInnerLeft, switchTop, switchInnerRight, switchBottom);
        mThumbDrawable.getPadding(mTempRect);
        final int thumbPos = (int) (mThumbPosition + 0.5f);
        int thumbLeft = switchInnerLeft - mTempRect.left + thumbPos;
        int thumbRight = switchInnerLeft + thumbPos + mThumbWidth + mTempRect.right;
        mThumbDrawable.setBounds(thumbLeft, switchTop, thumbRight, switchBottom);
        mThumbDrawable.draw(canvas);
        if (mTextColors != null) {
            mTextPaint.setColor(mTextColors.getColorForState(getDrawableState(),
                    mTextColors.getDefaultColor()));
        }
        mTextPaint.drawableState = getDrawableState();
        Layout switchText = getTargetCheckedState() ? mOnLayout : mOffLayout;
        canvas.translate((thumbLeft + thumbRight) / 2 - switchText.getWidth() / 2,
                (switchInnerTop + switchInnerBottom) / 2 - switchText.getHeight() / 2);
        switchText.draw(canvas);
        canvas.restoreToCount(saveState);
    }

    @Override
    public void onInitializeAccessibilityEvent(AccessibilityEvent event) {
        super.onInitializeAccessibilityEvent(event);
        event.setClassName(Switch.class.getName());
    }

    @Override
    public void onInitializeAccessibilityNodeInfo(AccessibilityNodeInfo info) {
        super.onInitializeAccessibilityNodeInfo(info);
        info.setClassName(Switch.class.getName());
        CharSequence switchText = isChecked() ? mTextOn : mTextOff;
        if (!TextUtils.isEmpty(switchText)) {
            CharSequence oldText = info.getText();
            if (TextUtils.isEmpty(oldText)) {
                info.setText(switchText);
            } else {
                StringBuilder newText = new StringBuilder();
                newText.append(oldText).append(' ').append(switchText);
                info.setText(newText);
            }
        }
    }

    @Override
    protected void onLayout(boolean changed, int left, int top, int right, int bottom) {
        super.onLayout(changed, left, top, right, bottom);
        mThumbPosition = isChecked() ? getThumbScrollRange() : 0;
        int switchRight = getWidth() - getPaddingRight();
        int switchLeft = switchRight - mSwitchWidth;
        int switchTop = 0;
        int switchBottom = 0;
        switch (getGravity() & Gravity.VERTICAL_GRAVITY_MASK) {
            default:
            case Gravity.TOP:
                switchTop = getPaddingTop();
                switchBottom = switchTop + mSwitchHeight;
                break;
            case Gravity.CENTER_VERTICAL:
                switchTop = (getPaddingTop() + getHeight() - getPaddingBottom()) / 2 -
                        mSwitchHeight / 2;
                switchBottom = switchTop + mSwitchHeight;
                break;
            case Gravity.BOTTOM:
                switchBottom = getHeight() - getPaddingBottom();
                switchTop = switchBottom - mSwitchHeight;
                break;
        }
        mSwitchLeft = switchLeft;
        mSwitchTop = switchTop;
        mSwitchBottom = switchBottom;
        mSwitchRight = switchRight;
    }

    @Override
    public void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        final int widthMode = MeasureSpec.getMode(widthMeasureSpec);
        final int heightMode = MeasureSpec.getMode(heightMeasureSpec);
        int widthSize = MeasureSpec.getSize(widthMeasureSpec);
        int heightSize = MeasureSpec.getSize(heightMeasureSpec);
        if (mOnLayout == null) {
            mOnLayout = makeLayout(mTextOn);
        }
        if (mOffLayout == null) {
            mOffLayout = makeLayout(mTextOff);
        }
        mTrackDrawable.getPadding(mTempRect);
        final int maxTextWidth = Math.max(mOnLayout.getWidth(), mOffLayout.getWidth());
        final int switchWidth = Math.max(mSwitchMinWidth,
                maxTextWidth * 2 + mThumbTextPadding * 4 + mTempRect.left + mTempRect.right);
        final int switchHeight = mTrackDrawable.getIntrinsicHeight();
        mThumbWidth = maxTextWidth + mThumbTextPadding * 2;
        switch (widthMode) {
            case MeasureSpec.AT_MOST:
                widthSize = Math.min(widthSize, switchWidth);
                break;
            case MeasureSpec.UNSPECIFIED:
                widthSize = switchWidth;
                break;
            case MeasureSpec.EXACTLY:
                break;
        }
        switch (heightMode) {
            case MeasureSpec.AT_MOST:
                heightSize = Math.min(heightSize, switchHeight);
                break;
            case MeasureSpec.UNSPECIFIED:
                heightSize = switchHeight;
                break;
            case MeasureSpec.EXACTLY:
                break;
        }
        mSwitchWidth = switchWidth;
        mSwitchHeight = switchHeight;
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
        final int measuredHeight = getMeasuredHeight();
        if (measuredHeight < switchHeight) {
            setMeasuredDimension(getMeasuredWidth(), switchHeight);
        }
    }

    @Override
    public void onPopulateAccessibilityEvent(AccessibilityEvent event) {
        super.onPopulateAccessibilityEvent(event);
        CharSequence text = isChecked() ? mOnLayout.getText() : mOffLayout.getText();
        if (!TextUtils.isEmpty(text)) {
            event.getText().add(text);
        }
    }

    @Override
    public boolean onTouchEvent(MotionEvent ev) {
        mVelocityTracker.addMovement(ev);
        switch (MotionEventCompat.getActionMasked(ev)) {
            case MotionEvent.ACTION_DOWN: {
                final float x = ev.getX();
                final float y = ev.getY();
                if (isEnabled() && hitThumb(x, y)) {
                    mTouchMode = TOUCH_MODE_DOWN;
                    mTouchX = x;
                    mTouchY = y;
                }
                return true;
            }
            case MotionEvent.ACTION_MOVE: {
                switch (mTouchMode) {
                    case TOUCH_MODE_IDLE:
                        break;
                    case TOUCH_MODE_DOWN: {
                        final float x = ev.getX();
                        final float y = ev.getY();
                        if (Math.abs(x - mTouchX) > mTouchSlop ||
                                Math.abs(y - mTouchY) > mTouchSlop) {
                            mTouchMode = TOUCH_MODE_DRAGGING;
                            getParent().requestDisallowInterceptTouchEvent(true);
                            mTouchX = x;
                            mTouchY = y;
                            return true;
                        }
                        break;
                    }
                    case TOUCH_MODE_DRAGGING: {
                        final float x = ev.getX();
                        final float dx = x - mTouchX;
                        float newPos = Math.max(0,
                                Math.min(mThumbPosition + dx, getThumbScrollRange()));
                        if (newPos != mThumbPosition) {
                            mThumbPosition = newPos;
                            mTouchX = x;
                            invalidate();
                        }
                        return true;
                    }
                }
                break;
            }
            case MotionEvent.ACTION_UP:
                if (mTouchMode == TOUCH_MODE_DOWN && mToggleWhenClick) {
                    toggle();
                    cancelSuperTouch(ev);
                    mTouchMode = TOUCH_MODE_IDLE;
                    mVelocityTracker.clear();
                    return true;
                }
            case MotionEvent.ACTION_CANCEL: {
                if (mTouchMode == TOUCH_MODE_DRAGGING) {
                    stopDrag(ev);
                    return true;
                }
                mTouchMode = TOUCH_MODE_IDLE;
                mVelocityTracker.clear();
                break;
            }
        }
        return super.onTouchEvent(ev);
    }

    @Override
    public void setChecked(boolean checked) {
        super.setChecked(checked);
        mThumbPosition = checked ? getThumbScrollRange() : 0;
        invalidate();
    }

    public void setSwitchMinWidth(int pixels) {
        mSwitchMinWidth = pixels;
        requestLayout();
    }

    public void setSwitchPadding(int pixels) {
        mSwitchPadding = pixels;
        requestLayout();
    }

    public void setSwitchTextAppearance(Context context, int resid) {
        TypedArray appearance =
                context.obtainStyledAttributes(resid,
                        R.styleable.TextAppearance);
        ColorStateList colors;
        int ts;
        colors = appearance.getColorStateList(R.styleable.
                TextAppearance_android_textColor);
        if (colors != null) {
            mTextColors = colors;
        } else {
            mTextColors = getTextColors();
        }
        ts = appearance.getDimensionPixelSize(R.styleable.
                TextAppearance_android_textSize, 0);
        if (ts != 0) {
            if (ts != mTextPaint.getTextSize()) {
                mTextPaint.setTextSize(ts);
                requestLayout();
            }
        }
        int typefaceIndex, styleIndex;
        typefaceIndex = appearance.getInt(R.styleable.
                TextAppearance_android_typeface, -1);
        styleIndex = appearance.getInt(R.styleable.
                TextAppearance_android_textStyle, -1);
        setSwitchTypefaceByIndex(typefaceIndex, styleIndex);
        boolean allCaps = appearance.getBoolean(R.styleable.
                TextAppearance_android_textAllCaps, false);
        if (allCaps) {
            mSwitchTransformationMethod = new AllCapsTransformationMethod(getContext());
            mSwitchTransformationMethod.setLengthChangesAllowed(true);
        } else {
            mSwitchTransformationMethod = null;
        }
        appearance.recycle();
    }

    public void setSwitchTypeface(Typeface tf) {
        if (mTextPaint.getTypeface() != tf) {
            mTextPaint.setTypeface(tf);
            requestLayout();
            invalidate();
        }
    }

    public void setSwitchTypeface(Typeface tf, int style) {
        if (style > 0) {
            if (tf == null) {
                tf = Typeface.defaultFromStyle(style);
            } else {
                tf = Typeface.create(tf, style);
            }
            setSwitchTypeface(tf);
            int typefaceStyle = tf != null ? tf.getStyle() : 0;
            int need = style & ~typefaceStyle;
            mTextPaint.setFakeBoldText((need & Typeface.BOLD) != 0);
            mTextPaint.setTextSkewX((need & Typeface.ITALIC) != 0 ? -0.25f : 0);
        } else {
            mTextPaint.setFakeBoldText(false);
            mTextPaint.setTextSkewX(0);
            setSwitchTypeface(tf);
        }
    }

    private void setSwitchTypefaceByIndex(int typefaceIndex, int styleIndex) {
        Typeface tf = null;
        switch (typefaceIndex) {
            case SANS:
                tf = Typeface.SANS_SERIF;
                break;
            case SERIF:
                tf = Typeface.SERIF;
                break;
            case MONOSPACE:
                tf = Typeface.MONOSPACE;
                break;
        }
        setSwitchTypeface(tf, styleIndex);
    }

    public void setTextOff(CharSequence textOff) {
        mTextOff = textOff;
        requestLayout();
    }

    public void setTextOn(CharSequence textOn) {
        mTextOn = textOn;
        requestLayout();
    }

    public void setThumbDrawable(Drawable thumb) {
        mThumbDrawable = thumb;
        requestLayout();
    }

    public void setThumbResource(int resId) {
        setThumbDrawable(getContext().getResources().getDrawable(resId));
    }

    public void setThumbTextPadding(int pixels) {
        mThumbTextPadding = pixels;
        requestLayout();
    }

    public void setToggleWhenClick(boolean mToggleWhenClick) {
        this.mToggleWhenClick = mToggleWhenClick;
    }

    public void setTrackDrawable(Drawable track) {
        mTrackDrawable = track;
        requestLayout();
    }

    public void setTrackResource(int resId) {
        setTrackDrawable(getContext().getResources().getDrawable(resId));
    }

    private void stopDrag(MotionEvent ev) {
        mTouchMode = TOUCH_MODE_IDLE;
        boolean commitChange = ev.getAction() == MotionEvent.ACTION_UP && isEnabled();
        cancelSuperTouch(ev);
        if (commitChange) {
            boolean newState;
            mVelocityTracker.computeCurrentVelocity(1000);
            float xvel = mVelocityTracker.getXVelocity();
            if (Math.abs(xvel) > mMinFlingVelocity) {
                newState = xvel > 0;
            } else {
                newState = getTargetCheckedState();
            }
            animateThumbToCheckedState(newState);
        } else {
            animateThumbToCheckedState(isChecked());
        }
    }

    @Override
    protected boolean verifyDrawable(Drawable who) {
        return super.verifyDrawable(who) || who == mThumbDrawable || who == mTrackDrawable;
    }
}
