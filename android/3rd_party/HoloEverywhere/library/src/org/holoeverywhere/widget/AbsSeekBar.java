
package org.holoeverywhere.widget;

import org.holoeverywhere.R;
import org.holoeverywhere.internal._View;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.util.AttributeSet;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.ViewConfiguration;
import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityNodeInfo;

public abstract class AbsSeekBar extends ProgressBar {
    private static final int NO_ALPHA = 0xFF;
    private float mDisabledAlpha;
    private boolean mIsDragging;
    boolean mIsUserSeekable = true;
    private int mKeyProgressIncrement = 1;
    private int mScaledTouchSlop;
    private Drawable mThumb;
    private int mThumbOffset;
    private float mTouchDownX;
    float mTouchProgressOffset;

    public AbsSeekBar(Context context) {
        super(context);
    }

    public AbsSeekBar(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public AbsSeekBar(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        TypedArray a = context.obtainStyledAttributes(attrs,
                R.styleable.SeekBar, defStyle, 0);
        Drawable thumb = a.getDrawable(R.styleable.SeekBar_android_thumb);
        setThumb(thumb);
        int thumbOffset = a.getDimensionPixelOffset(
                R.styleable.SeekBar_android_thumbOffset, getThumbOffset());
        setThumbOffset(thumbOffset);
        mDisabledAlpha = a.getFloat(R.styleable.SeekBar_android_disabledAlpha,
                0.5f);
        a.recycle();
        mScaledTouchSlop = ViewConfiguration.get(context).getScaledTouchSlop();
    }

    private void attemptClaimDrag() {
        if (getParent() != null) {
            getParent().requestDisallowInterceptTouchEvent(true);
        }
    }

    @Override
    protected void drawableStateChanged() {
        super.drawableStateChanged();
        Drawable progressDrawable = getProgressDrawable();
        if (progressDrawable != null) {
            progressDrawable.setAlpha(isEnabled() ? AbsSeekBar.NO_ALPHA
                    : (int) (AbsSeekBar.NO_ALPHA * mDisabledAlpha));
        }
        if (mThumb != null && mThumb.isStateful()) {
            int[] state = getDrawableState();
            mThumb.setState(state);
        }
    }

    public int getKeyProgressIncrement() {
        return mKeyProgressIncrement;
    }

    public Drawable getThumb() {
        return mThumb;
    }

    public int getThumbOffset() {
        return mThumbOffset;
    }

    public boolean isInScrollingContainer() {
        return false;
    }

    @Override
    @SuppressLint("NewApi")
    public void jumpDrawablesToCurrentState() {
        super.jumpDrawablesToCurrentState();
        if (mThumb != null) {
            mThumb.jumpToCurrentState();
        }
    }

    @Override
    protected synchronized void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        if (mThumb != null) {
            canvas.save();
            canvas.translate(getPaddingLeft() - mThumbOffset, getPaddingTop());
            mThumb.draw(canvas);
            canvas.restore();
        }
    }

    @Override
    @SuppressLint("NewApi")
    public void onInitializeAccessibilityEvent(AccessibilityEvent event) {
        super.onInitializeAccessibilityEvent(event);
        event.setClassName(AbsSeekBar.class.getName());
    }

    @Override
    @SuppressLint("NewApi")
    public void onInitializeAccessibilityNodeInfo(AccessibilityNodeInfo info) {
        super.onInitializeAccessibilityNodeInfo(info);
        info.setClassName(AbsSeekBar.class.getName());
        if (isEnabled()) {
            final int progress = getProgress();
            if (progress > 0) {
                info.addAction(AccessibilityNodeInfo.ACTION_SCROLL_BACKWARD);
            }
            if (progress < getMax()) {
                info.addAction(AccessibilityNodeInfo.ACTION_SCROLL_FORWARD);
            }
        }
    }

    void onKeyChange() {
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (isEnabled()) {
            int progress = getProgress();
            switch (keyCode) {
                case KeyEvent.KEYCODE_DPAD_LEFT:
                    if (progress <= 0) {
                        break;
                    }
                    setProgress(progress - mKeyProgressIncrement, true);
                    onKeyChange();
                    return true;
                case KeyEvent.KEYCODE_DPAD_RIGHT:
                    if (progress >= getMax()) {
                        break;
                    }
                    setProgress(progress + mKeyProgressIncrement, true);
                    onKeyChange();
                    return true;
            }
        }
        return super.onKeyDown(keyCode, event);
    }

    @Override
    protected synchronized void onMeasure(int widthMeasureSpec,
            int heightMeasureSpec) {
        Drawable d = getCurrentDrawable();
        int thumbHeight = mThumb == null ? 0 : mThumb.getIntrinsicHeight();
        int dw = 0;
        int dh = 0;
        if (d != null) {
            dw = Math
                    .max(mMinWidth, Math.min(mMaxWidth, d.getIntrinsicWidth()));
            dh = Math.max(mMinHeight,
                    Math.min(mMaxHeight, d.getIntrinsicHeight()));
            dh = Math.max(thumbHeight, dh);
        }
        dw += getPaddingLeft() + getPaddingRight();
        dh += getPaddingTop() + getPaddingBottom();
        setMeasuredDimension(
                _View.supportResolveSizeAndState(dw, widthMeasureSpec, 0),
                _View.supportResolveSizeAndState(dh, heightMeasureSpec, 0));
    }

    @Override
    public void onProgressRefresh(float scale, boolean fromUser) {
        super.onProgressRefresh(scale, fromUser);
        Drawable thumb = mThumb;
        if (thumb != null) {
            setThumbPos(getWidth(), thumb, scale, Integer.MIN_VALUE);
            invalidate();
        }
    }

    @Override
    protected void onSizeChanged(int w, int h, int oldw, int oldh) {
        updateThumbPos(w, h);
    }

    void onStartTrackingTouch() {
        mIsDragging = true;
    }

    void onStopTrackingTouch() {
        mIsDragging = false;
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        if (!mIsUserSeekable || !isEnabled()) {
            return false;
        }
        switch (event.getAction()) {
            case MotionEvent.ACTION_DOWN:
                if (isInScrollingContainer()) {
                    mTouchDownX = event.getX();
                } else {
                    setPressed(true);
                    if (mThumb != null) {
                        invalidate(mThumb.getBounds());
                    }
                    onStartTrackingTouch();
                    trackTouchEvent(event);
                    attemptClaimDrag();
                }
                break;
            case MotionEvent.ACTION_MOVE:
                if (mIsDragging) {
                    trackTouchEvent(event);
                } else {
                    final float x = event.getX();
                    if (Math.abs(x - mTouchDownX) > mScaledTouchSlop) {
                        setPressed(true);
                        if (mThumb != null) {
                            invalidate(mThumb.getBounds());
                        }
                        onStartTrackingTouch();
                        trackTouchEvent(event);
                        attemptClaimDrag();
                    }
                }
                break;
            case MotionEvent.ACTION_UP:
                if (mIsDragging) {
                    trackTouchEvent(event);
                    onStopTrackingTouch();
                    setPressed(false);
                } else {
                    onStartTrackingTouch();
                    trackTouchEvent(event);
                    onStopTrackingTouch();
                }
                invalidate();
                break;
            case MotionEvent.ACTION_CANCEL:
                if (mIsDragging) {
                    onStopTrackingTouch();
                    setPressed(false);
                }
                invalidate();
                break;
        }
        return true;
    }

    @Override
    public boolean performAccessibilityAction(int action, Bundle arguments) {
        if (super.performAccessibilityAction(action, arguments)) {
            return true;
        }
        if (!isEnabled()) {
            return false;
        }
        final int progress = getProgress();
        final int increment = Math.max(1, Math.round((float) getMax() / 5));
        switch (action) {
            case AccessibilityNodeInfo.ACTION_SCROLL_BACKWARD: {
                if (progress <= 0) {
                    return false;
                }
                setProgress(progress - increment, true);
                onKeyChange();
                return true;
            }
            case AccessibilityNodeInfo.ACTION_SCROLL_FORWARD: {
                if (progress >= getMax()) {
                    return false;
                }
                setProgress(progress + increment, true);
                onKeyChange();
                return true;
            }
        }
        return false;
    }

    public void setKeyProgressIncrement(int increment) {
        mKeyProgressIncrement = increment < 0 ? -increment : increment;
    }

    @Override
    public synchronized void setMax(int max) {
        super.setMax(max);
        if (mKeyProgressIncrement == 0 || getMax() / mKeyProgressIncrement > 20) {
            setKeyProgressIncrement(Math.max(1,
                    Math.round((float) getMax() / 20)));
        }
    }

    public void setThumb(Drawable thumb) {
        boolean needUpdate;
        if (mThumb != null && thumb != mThumb) {
            mThumb.setCallback(null);
            needUpdate = true;
        } else {
            needUpdate = false;
        }
        if (thumb != null) {
            thumb.setCallback(this);
            mThumbOffset = thumb.getIntrinsicWidth() / 2;
            if (needUpdate
                    && (thumb.getIntrinsicWidth() != mThumb.getIntrinsicWidth() || thumb
                            .getIntrinsicHeight() != mThumb
                            .getIntrinsicHeight())) {
                requestLayout();
            }
        }
        mThumb = thumb;
        invalidate();
        if (needUpdate) {
            updateThumbPos(getWidth(), getHeight());
            if (thumb != null && thumb.isStateful()) {
                int[] state = getDrawableState();
                thumb.setState(state);
            }
        }
    }

    public void setThumbOffset(int thumbOffset) {
        mThumbOffset = thumbOffset;
        invalidate();
    }

    private void setThumbPos(int w, Drawable thumb, float scale, int gap) {
        int available = w - getPaddingLeft() - getPaddingRight();
        int thumbWidth = thumb.getIntrinsicWidth();
        int thumbHeight = thumb.getIntrinsicHeight();
        available -= thumbWidth;
        available += mThumbOffset * 2;
        int thumbPos = (int) (scale * available);
        int topBound, bottomBound;
        if (gap == Integer.MIN_VALUE) {
            Rect oldBounds = thumb.getBounds();
            topBound = oldBounds.top;
            bottomBound = oldBounds.bottom;
        } else {
            topBound = gap;
            bottomBound = gap + thumbHeight;
        }
        thumb.setBounds(thumbPos, topBound, thumbPos + thumbWidth, bottomBound);
    }

    private void trackTouchEvent(MotionEvent event) {
        final int width = getWidth();
        final int available = width - getPaddingLeft() - getPaddingRight();
        int x = (int) event.getX();
        float scale;
        float progress = 0;
        if (x < getPaddingLeft()) {
            scale = 0.0f;
        } else if (x > width - getPaddingRight()) {
            scale = 1.0f;
        } else {
            scale = (float) (x - getPaddingLeft()) / (float) available;
            progress = mTouchProgressOffset;
        }
        final int max = getMax();
        progress += scale * max;
        setProgress((int) progress, true);
    }

    private void updateThumbPos(int w, int h) {
        Drawable d = getCurrentDrawable();
        Drawable thumb = mThumb;
        int thumbHeight = thumb == null ? 0 : thumb.getIntrinsicHeight();
        int trackHeight = Math.min(mMaxHeight, h - getPaddingTop()
                - getPaddingBottom());
        int max = getMax();
        float scale = max > 0 ? (float) getProgress() / (float) max : 0;
        if (thumbHeight > trackHeight) {
            if (thumb != null) {
                setThumbPos(w, thumb, scale, 0);
            }
            int gapForCenteringTrack = (thumbHeight - trackHeight) / 2;
            if (d != null) {
                d.setBounds(0, gapForCenteringTrack, w - getPaddingRight()
                        - getPaddingLeft(), h - getPaddingBottom()
                        - gapForCenteringTrack - getPaddingTop());
            }
        } else {
            if (d != null) {
                d.setBounds(0, 0, w - getPaddingRight() - getPaddingLeft(), h
                        - getPaddingBottom() - getPaddingTop());
            }
            int gap = (trackHeight - thumbHeight) / 2;
            if (thumb != null) {
                setThumbPos(w, thumb, scale, gap);
            }
        }
    }

    @Override
    protected boolean verifyDrawable(Drawable who) {
        return who == mThumb || super.verifyDrawable(who);
    }
}
