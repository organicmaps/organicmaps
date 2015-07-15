package com.cocosw.bottomsheet;

import android.annotation.TargetApi;
import android.content.Context;
import android.os.Build;
import android.support.annotation.NonNull;
import android.support.v4.view.MotionEventCompat;
import android.support.v4.view.ViewCompat;
import android.support.v4.widget.ViewDragHelper;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;
import android.widget.AbsListView;
import android.widget.FrameLayout;

/**
 * Project: gradle
 * Created by LiaoKai(soarcn) on 2014/11/25.
 */
class ClosableSlidingLayout extends FrameLayout {

    private final float MINVEL;
    private ViewDragHelper mDragHelper;
    private SlideListener mListener;
    private int height;
    private int top;
    private int mActivePointerId;
    private boolean mIsBeingDragged;
    private float mInitialMotionY;
    private static final int INVALID_POINTER = -1;
    View mTarget;

    private boolean collapsible = false;
    private float yDiff;

    boolean swipeable = true;

    public ClosableSlidingLayout(Context context) {
        this(context, null);
    }

    public ClosableSlidingLayout(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    @TargetApi(Build.VERSION_CODES.HONEYCOMB)
    public ClosableSlidingLayout(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        mDragHelper = ViewDragHelper.create(this, 0.8f, new ViewDragCallback());
        MINVEL = getResources().getDisplayMetrics().density * 400;
    }

    @Override
    public boolean onInterceptTouchEvent(@NonNull MotionEvent event) {
        final int action = MotionEventCompat.getActionMasked(event);

        if (!isEnabled() || canChildScrollUp()) {
            // Fail fast if we're not in a state where a swipe is possible
            return false;
        }

        if (action == MotionEvent.ACTION_CANCEL || action == MotionEvent.ACTION_UP) {
            mActivePointerId = INVALID_POINTER;
            mIsBeingDragged = false;
            if (collapsible && -yDiff > mDragHelper.getTouchSlop()) {
                expand(mDragHelper.getCapturedView(), 0);
            }
            mDragHelper.cancel();
            return false;
        }

        switch (action) {
            case MotionEvent.ACTION_DOWN:
                height = getChildAt(0).getHeight();
                top = getChildAt(0).getTop();
                mActivePointerId = MotionEventCompat.getPointerId(event, 0);
                mIsBeingDragged = false;
                final float initialMotionY = getMotionEventY(event, mActivePointerId);
                if (initialMotionY == -1) {
                    return false;
                }
                mInitialMotionY = initialMotionY;
                yDiff = 0;
                break;
            case MotionEvent.ACTION_MOVE:
                if (mActivePointerId == INVALID_POINTER) {
                    return false;
                }
                final float y = getMotionEventY(event, mActivePointerId);
                if (y == -1) {
                    return false;
                }
                yDiff = y - mInitialMotionY;
                if (swipeable && yDiff > mDragHelper.getTouchSlop() && !mIsBeingDragged) {
                    mIsBeingDragged = true;
                    mDragHelper.captureChildView(getChildAt(0), 0);
                }
                break;
        }
        mDragHelper.shouldInterceptTouchEvent(event);
        return mIsBeingDragged;
    }

    @Override
    public void requestDisallowInterceptTouchEvent(boolean b) {
        // Nope.
    }

    /**
     * @return Whether it is possible for the child view of this layout to
     * scroll up. Override this if the child view is a custom view.
     */
    private boolean canChildScrollUp() {
        if (android.os.Build.VERSION.SDK_INT < 14) {
            if (mTarget instanceof AbsListView) {
                final AbsListView absListView = (AbsListView) mTarget;
                return absListView.getChildCount() > 0
                        && (absListView.getFirstVisiblePosition() > 0 || absListView.getChildAt(0)
                        .getTop() < absListView.getPaddingTop());
            } else {
                return mTarget.getScrollY() > 0;
            }
        } else {
            return ViewCompat.canScrollVertically(mTarget, -1);
        }
    }

    private float getMotionEventY(MotionEvent ev, int activePointerId) {
        final int index = MotionEventCompat.findPointerIndex(ev, activePointerId);
        if (index < 0) {
            return -1;
        }
        return MotionEventCompat.getY(ev, index);
    }

    @Override
    public boolean onTouchEvent(MotionEvent ev) {
        if (!isEnabled() || canChildScrollUp()) {
            return super.onTouchEvent(ev);
        }

        try {
            if (swipeable)
                mDragHelper.processTouchEvent(ev);
        } catch (Exception ignored) {
        }
        return true;
    }

    @Override
    public void computeScroll() {
        if (mDragHelper.continueSettling(true)) {
            ViewCompat.postInvalidateOnAnimation(this);
        }
    }

    public void setSlideListener(SlideListener listener) {
        mListener = listener;
    }

    void setCollapsible(boolean collapsible) {
        this.collapsible = collapsible;
    }

    /**
     * Callback
     */
    private class ViewDragCallback extends ViewDragHelper.Callback {


        @Override
        public boolean tryCaptureView(View child, int pointerId) {
            return true;
        }

        @Override
        public void onViewReleased(View releasedChild, float xvel, float yvel) {
            if (yvel > MINVEL) {
                dismiss(releasedChild, yvel);
            } else {
                if (releasedChild.getTop() >= top + height / 2) {
                    dismiss(releasedChild, yvel);
                } else {
                    mDragHelper.smoothSlideViewTo(releasedChild, 0, top);
                }
            }
            ViewCompat.postInvalidateOnAnimation(ClosableSlidingLayout.this);
        }

        @Override
        public void onViewPositionChanged(View changedView, int left, int top, int dx, int dy) {
            if (Build.VERSION.SDK_INT < Build.VERSION_CODES.HONEYCOMB) {
                invalidate();
            }
            if (height-top <1 && mListener!=null) {
                mListener.onClosed();
            }
        }

        @Override
        public int clampViewPositionVertical(View child, int top, int dy) {
            return Math.max(top, ClosableSlidingLayout.this.top);
        }
    }

    private void expand(View releasedChild, float yvel) {
        if (mListener != null) {
            mListener.onOpened();
        }
    }

    private void dismiss(View view, float yvel) {
        mDragHelper.smoothSlideViewTo(view, 0, top + height);
        mDragHelper.cancel();
        ViewCompat.postInvalidateOnAnimation(ClosableSlidingLayout.this);
    }


    /**
     * set listener
     */
    interface SlideListener {
        void onClosed();

        void onOpened();
    }

}