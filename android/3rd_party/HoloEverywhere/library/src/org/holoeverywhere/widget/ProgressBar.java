
package org.holoeverywhere.widget;

import java.util.ArrayList;

import org.holoeverywhere.R;
import org.holoeverywhere.drawable.DrawableCompat;
import org.holoeverywhere.internal._View;
import org.holoeverywhere.util.Pool;
import org.holoeverywhere.util.Poolable;
import org.holoeverywhere.util.PoolableManager;
import org.holoeverywhere.util.Pools;
import org.holoeverywhere.util.ReflectHelper;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Bitmap;
import android.graphics.BitmapShader;
import android.graphics.Canvas;
import android.graphics.Rect;
import android.graphics.Shader;
import android.graphics.drawable.Animatable;
import android.graphics.drawable.AnimationDrawable;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.ClipDrawable;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.LayerDrawable;
import android.graphics.drawable.ShapeDrawable;
import android.graphics.drawable.StateListDrawable;
import android.graphics.drawable.shapes.RoundRectShape;
import android.graphics.drawable.shapes.Shape;
import android.os.Parcel;
import android.os.Parcelable;
import android.util.AttributeSet;
import android.view.Gravity;
import android.view.View;
import android.view.ViewDebug;
import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityManager;
import android.view.accessibility.AccessibilityNodeInfo;
import android.view.animation.AlphaAnimation;
import android.view.animation.Animation;
import android.view.animation.AnimationUtils;
import android.view.animation.Interpolator;
import android.view.animation.LinearInterpolator;
import android.view.animation.Transformation;

public class ProgressBar extends android.widget.ProgressBar {
    private class AccessibilityEventSender implements Runnable {
        @Override
        public void run() {
            sendAccessibilityEvent(AccessibilityEvent.TYPE_VIEW_SELECTED);
        }
    }

    private static class RefreshData implements Poolable<RefreshData> {
        private static final int POOL_MAX = 24;
        private static final Pool<RefreshData> sPool = Pools
                .synchronizedPool(Pools.finitePool(
                        new PoolableManager<RefreshData>() {
                            @Override
                            public RefreshData newInstance() {
                                return new RefreshData();
                            }

                            @Override
                            public void onAcquired(RefreshData element) {
                            }

                            @Override
                            public void onReleased(RefreshData element) {
                            }
                        }, RefreshData.POOL_MAX));

        public static RefreshData obtain(int id, int progress, boolean fromUser) {
            RefreshData rd = RefreshData.sPool.acquire();
            rd.id = id;
            rd.progress = progress;
            rd.fromUser = fromUser;
            return rd;
        }

        public boolean fromUser;
        public int id;
        private boolean mIsPooled;
        private RefreshData mNext;

        public int progress;

        @Override
        public RefreshData getNextPoolable() {
            return mNext;
        }

        @Override
        public boolean isPooled() {
            return mIsPooled;
        }

        public void recycle() {
            RefreshData.sPool.release(this);
        }

        @Override
        public void setNextPoolable(RefreshData element) {
            mNext = element;
        }

        @Override
        public void setPooled(boolean isPooled) {
            mIsPooled = isPooled;
        }
    }

    private class RefreshProgressRunnable implements Runnable {
        @Override
        public void run() {
            synchronized (ProgressBar.this) {
                final int count = mRefreshData.size();
                for (int i = 0; i < count; i++) {
                    final RefreshData rd = mRefreshData.get(i);
                    doRefreshProgress(rd.id, rd.progress, rd.fromUser, true);
                    rd.recycle();
                }
                mRefreshData.clear();
                mRefreshIsPosted = false;
            }
        }
    }

    protected static class SavedState extends BaseSavedState {
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
        int progress;

        int secondaryProgress;

        protected SavedState(Parcel in) {
            super(in);
            progress = in.readInt();
            secondaryProgress = in.readInt();
        }

        protected SavedState(Parcelable superState) {
            super(superState);
        }

        @Override
        public void writeToParcel(Parcel out, int flags) {
            super.writeToParcel(out, flags);
            out.writeInt(progress);
            out.writeInt(secondaryProgress);
        }
    }

    private static final int MAX_LEVEL = 10000;
    private static final int TIMEOUT_SEND_ACCESSIBILITY_EVENT = 200;
    private AccessibilityEventSender mAccessibilityEventSender;
    private AlphaAnimation mAnimation;
    private boolean mAttached;
    private int mBehavior;
    private Drawable mCurrentDrawable;
    private int mDuration;
    private boolean mHasAnimation;
    private boolean mIndeterminate;
    private Drawable mIndeterminateDrawable;
    private boolean mInDrawing;
    private Interpolator mInterpolator;
    private int mMax;
    protected int mMinWidth, mMaxWidth, mMinHeight, mMaxHeight;
    private boolean mNoInvalidate;
    private boolean mOnlyIndeterminate;
    private int mProgress;
    private Drawable mProgressDrawable;
    private final ArrayList<RefreshData> mRefreshData = new ArrayList<RefreshData>();
    private boolean mRefreshIsPosted;
    private RefreshProgressRunnable mRefreshProgressRunnable;
    private Bitmap mSampleTile;
    private int mSecondaryProgress;
    private boolean mShouldStartAnimationDrawable;
    private Transformation mTransformation;
    private long mUiThreadId;

    public ProgressBar(Context context) {
        this(context, null);
    }

    public ProgressBar(Context context, AttributeSet attrs) {
        this(context, attrs, android.R.attr.progressBarStyle);
    }

    public ProgressBar(Context context, AttributeSet attrs, int defStyle) {
        this(context, attrs, defStyle, R.style.Holo_ProgressBar);
    }

    public ProgressBar(Context context, AttributeSet attrs, int defStyle,
            int styleRes) {
        super(context, attrs, defStyle);
        mUiThreadId = Thread.currentThread().getId();
        initProgressBar();
        TypedArray a = context.obtainStyledAttributes(attrs,
                R.styleable.ProgressBar, defStyle, styleRes);
        mNoInvalidate = true;

        Drawable drawable = a.getDrawable(R.styleable.ProgressBar_android_progressDrawable);
        if (drawable != null) {
            drawable = tileify(drawable, false);
            setProgressDrawable(drawable);
        }
        mDuration = a.getInt(
                R.styleable.ProgressBar_android_indeterminateDuration,
                mDuration);
        mMinWidth = a.getDimensionPixelSize(
                R.styleable.ProgressBar_android_minWidth, mMinWidth);
        mMaxWidth = a.getDimensionPixelSize(
                R.styleable.ProgressBar_android_maxWidth, mMaxWidth);
        mMinHeight = a.getDimensionPixelSize(
                R.styleable.ProgressBar_android_minHeight, mMinHeight);
        mMaxHeight = a.getDimensionPixelSize(
                R.styleable.ProgressBar_android_maxHeight, mMaxHeight);
        mBehavior = a.getInt(
                R.styleable.ProgressBar_android_indeterminateBehavior,
                mBehavior);
        final int resID = a.getResourceId(
                R.styleable.ProgressBar_android_interpolator,
                android.R.anim.linear_interpolator);
        if (resID > 0) {
            setInterpolator(context, resID);
        }
        setMax(a.getInt(R.styleable.ProgressBar_android_max, mMax));
        setProgress(a.getInt(R.styleable.ProgressBar_android_progress,
                mProgress));
        setSecondaryProgress(a.getInt(
                R.styleable.ProgressBar_android_secondaryProgress,
                mSecondaryProgress));
        drawable = DrawableCompat.getDrawable(a,
                R.styleable.ProgressBar_android_indeterminateDrawable);
        if (drawable != null) {
            drawable = tileifyIndeterminate(drawable);
            setIndeterminateDrawable(drawable);
        }
        mOnlyIndeterminate = a.getBoolean(
                R.styleable.ProgressBar_android_indeterminateOnly,
                mOnlyIndeterminate);
        mNoInvalidate = false;
        setIndeterminate(mOnlyIndeterminate
                || a.getBoolean(R.styleable.ProgressBar_android_indeterminate,
                        mIndeterminate));
        a.recycle();
    }

    private synchronized void doRefreshProgress(int id, int progress,
            boolean fromUser, boolean callBackToApp) {
        float scale = mMax > 0 ? (float) progress / (float) mMax : 0;
        final Drawable d = mCurrentDrawable;
        if (d != null) {
            Drawable progressDrawable = null;
            if (d instanceof LayerDrawable) {
                progressDrawable = ((LayerDrawable) d)
                        .findDrawableByLayerId(id);
            }
            final int level = (int) (scale * ProgressBar.MAX_LEVEL);
            (progressDrawable != null ? progressDrawable : d).setLevel(level);
        } else {
            invalidate();
        }
        if (callBackToApp && id == R.id.progress) {
            onProgressRefresh(scale, fromUser);
        }
    }

    @Override
    protected void drawableStateChanged() {
        super.drawableStateChanged();
        updateDrawableState();
    }

    protected Drawable getCurrentDrawable() {
        return mCurrentDrawable;
    }

    private Shape getDrawableShape() {
        final float[] roundedCorners = new float[] {
                5, 5, 5, 5, 5, 5, 5, 5
        };
        return new RoundRectShape(roundedCorners, null, null);
    }

    @Override
    public Drawable getIndeterminateDrawable() {
        return mIndeterminateDrawable;
    }

    @Override
    public Interpolator getInterpolator() {
        return mInterpolator;
    }

    @Override
    @ViewDebug.ExportedProperty(category = "progress")
    public synchronized int getMax() {
        return mMax;
    }

    @Override
    @ViewDebug.ExportedProperty(category = "progress")
    public synchronized int getProgress() {
        return mIndeterminate ? 0 : mProgress;
    }

    @Override
    public Drawable getProgressDrawable() {
        return mProgressDrawable;
    }

    public int getResolvedLayoutDirection() {
        return 0;
    }

    public int getResolvedLayoutDirection(Drawable who) {
        return who == mProgressDrawable || who == mIndeterminateDrawable ? getResolvedLayoutDirection()
                : 0;
    }

    @Override
    @ViewDebug.ExportedProperty(category = "progress")
    public synchronized int getSecondaryProgress() {
        return mIndeterminate ? 0 : mSecondaryProgress;
    }

    public synchronized final void incrementProgress(int diff) {
        setProgress(mProgress + diff);
    }

    public synchronized final void incrementSecondaryProgress(int diff) {
        setSecondaryProgress(mSecondaryProgress + diff);
    }

    private void initProgressBar() {
        mMax = 100;
        mProgress = 0;
        mSecondaryProgress = 0;
        mIndeterminate = false;
        mOnlyIndeterminate = false;
        mDuration = 4000;
        mBehavior = Animation.RESTART;
        mMinWidth = 24;
        mMaxWidth = 48;
        mMinHeight = 24;
        mMaxHeight = 48;
    }

    @Override
    public void invalidateDrawable(Drawable dr) {
        if (!mInDrawing) {
            if (verifyDrawable(dr)) {
                final Rect dirty = dr.getBounds();
                final int scrollX = getScrollX() + getPaddingLeft();
                final int scrollY = getScrollY() + getPaddingTop();

                invalidate(dirty.left + scrollX, dirty.top + scrollY,
                        dirty.right + scrollX, dirty.bottom + scrollY);
            } else {
                super.invalidateDrawable(dr);
            }
        }
    }

    @Override
    @ViewDebug.ExportedProperty(category = "progress")
    public synchronized boolean isIndeterminate() {
        return mIndeterminate;
    }

    @SuppressLint("NewApi")
    @Override
    public void jumpDrawablesToCurrentState() {
        super.jumpDrawablesToCurrentState();
        if (mProgressDrawable != null) {
            mProgressDrawable.jumpToCurrentState();
        }
        if (mIndeterminateDrawable != null) {
            mIndeterminateDrawable.jumpToCurrentState();
        }
    }

    @Override
    protected void onAttachedToWindow() {
        super.onAttachedToWindow();
        if (mIndeterminate) {
            startAnimation();
        }
        if (mRefreshData != null) {
            synchronized (this) {
                final int count = mRefreshData.size();
                for (int i = 0; i < count; i++) {
                    final RefreshData rd = mRefreshData.get(i);
                    doRefreshProgress(rd.id, rd.progress, rd.fromUser, true);
                    rd.recycle();
                }
                mRefreshData.clear();
            }
        }
        mAttached = true;
    }

    @Override
    protected void onDetachedFromWindow() {
        if (mIndeterminate) {
            stopAnimation();
        }
        if (mRefreshProgressRunnable != null) {
            removeCallbacks(mRefreshProgressRunnable);
        }
        if (mRefreshProgressRunnable != null && mRefreshIsPosted) {
            removeCallbacks(mRefreshProgressRunnable);
        }
        if (mAccessibilityEventSender != null) {
            removeCallbacks(mAccessibilityEventSender);
        }
        super.onDetachedFromWindow();
        mAttached = false;
    }

    @Override
    protected synchronized void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        Drawable d = mCurrentDrawable;
        if (d != null) {
            canvas.save();
            canvas.translate(getPaddingLeft(), getPaddingTop());
            long time = getDrawingTime();
            if (mHasAnimation) {
                mAnimation.getTransformation(time, mTransformation);
                float scale = mTransformation.getAlpha();
                try {
                    mInDrawing = true;
                    d.setLevel((int) (scale * ProgressBar.MAX_LEVEL));
                } finally {
                    mInDrawing = false;
                }
                postInvalidate();
            }
            d.draw(canvas);
            canvas.restore();
            if (mShouldStartAnimationDrawable && d instanceof Animatable) {
                ((Animatable) d).start();
                mShouldStartAnimationDrawable = false;
            }
        }
    }

    @SuppressLint("NewApi")
    @Override
    public void onInitializeAccessibilityEvent(AccessibilityEvent event) {
        super.onInitializeAccessibilityEvent(event);
        event.setClassName(ProgressBar.class.getName());
        event.setItemCount(mMax);
        event.setCurrentItemIndex(mProgress);
    }

    @Override
    @SuppressLint("NewApi")
    public void onInitializeAccessibilityNodeInfo(AccessibilityNodeInfo info) {
        super.onInitializeAccessibilityNodeInfo(info);
        info.setClassName(ProgressBar.class.getName());
    }

    @Override
    protected synchronized void onMeasure(int widthMeasureSpec,
            int heightMeasureSpec) {
        Drawable d = mCurrentDrawable;
        int dw = 0;
        int dh = 0;
        if (d != null) {
            dw = Math
                    .max(mMinWidth, Math.min(mMaxWidth, d.getIntrinsicWidth()));
            dh = Math.max(mMinHeight,
                    Math.min(mMaxHeight, d.getIntrinsicHeight()));
        }
        updateDrawableState();
        dw += getPaddingLeft() + getPaddingRight();
        dh += getPaddingTop() + getPaddingBottom();
        setMeasuredDimension(
                _View.supportResolveSizeAndState(dw, widthMeasureSpec, 0),
                _View.supportResolveSizeAndState(dh, heightMeasureSpec, 0));
    }

    protected void onProgressRefresh(float scale, boolean fromUser) {
        try {
            if (((AccessibilityManager) AccessibilityManager.class.getMethod(
                    "getInstance", Context.class).invoke(null, getContext()))
                    .isEnabled()) {
                scheduleAccessibilityEventSender();
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    @Override
    public void onRestoreInstanceState(Parcelable state) {
        SavedState ss = (SavedState) state;
        super.onRestoreInstanceState(ss.getSuperState());
        setProgress(ss.progress);
        setSecondaryProgress(ss.secondaryProgress);
    }

    @Override
    public Parcelable onSaveInstanceState() {
        Parcelable superState = super.onSaveInstanceState();
        SavedState ss = new SavedState(superState);
        ss.progress = mProgress;
        ss.secondaryProgress = mSecondaryProgress;
        return ss;
    }

    @Override
    protected void onSizeChanged(int w, int h, int oldw, int oldh) {
        updateDrawableBounds(w, h);
    }

    @Override
    public void onVisibilityChanged(View changedView, int visibility) {
        super.onVisibilityChanged(changedView, visibility);
        if (mIndeterminate) {
            if (visibility == android.view.View.GONE
                    || visibility == android.view.View.INVISIBLE) {
                stopAnimation();
            } else {
                startAnimation();
            }
        }
    }

    @Override
    public void postInvalidate() {
        if (!mNoInvalidate) {
            super.postInvalidate();
        }
    }

    private synchronized void refreshProgress(int id, int progress,
            boolean fromUser) {
        if (mUiThreadId == Thread.currentThread().getId()) {
            doRefreshProgress(id, progress, fromUser, true);
        } else if (mRefreshData != null) {
            if (mRefreshProgressRunnable == null) {
                mRefreshProgressRunnable = new RefreshProgressRunnable();
            }

            final RefreshData rd = RefreshData.obtain(id, progress, fromUser);
            mRefreshData.add(rd);
            if (mAttached && !mRefreshIsPosted) {
                post(mRefreshProgressRunnable);
                mRefreshIsPosted = true;
            }
        }
    }

    private void scheduleAccessibilityEventSender() {
        if (mAccessibilityEventSender == null) {
            mAccessibilityEventSender = new AccessibilityEventSender();
        } else {
            removeCallbacks(mAccessibilityEventSender);
        }
        postDelayed(mAccessibilityEventSender,
                ProgressBar.TIMEOUT_SEND_ACCESSIBILITY_EVENT);
    }

    @Override
    public synchronized void setIndeterminate(boolean indeterminate) {
        if ((!mOnlyIndeterminate || !mIndeterminate)
                && indeterminate != mIndeterminate) {
            mIndeterminate = indeterminate;

            if (indeterminate) {
                // swap between indeterminate and regular backgrounds
                mCurrentDrawable = mIndeterminateDrawable;
                startAnimation();
            } else {
                mCurrentDrawable = mProgressDrawable;
                stopAnimation();
            }
        }
    }

    @Override
    public void setIndeterminateDrawable(Drawable d) {
        if (d != null) {
            d.setCallback(this);
        }
        mIndeterminateDrawable = d;
        if (mIndeterminate) {
            mCurrentDrawable = d;
            postInvalidate();
        }
    }

    @Override
    public void setInterpolator(Context context, int resID) {
        setInterpolator(AnimationUtils.loadInterpolator(context, resID));
    }

    @Override
    public void setInterpolator(Interpolator interpolator) {
        mInterpolator = interpolator;
    }

    @Override
    public synchronized void setMax(int max) {
        if (max < 0) {
            max = 0;
        }
        if (max != mMax) {
            mMax = max;
            postInvalidate();

            if (mProgress > max) {
                mProgress = max;
            }
            refreshProgress(R.id.progress, mProgress, false);
        }
    }

    @Override
    public synchronized void setProgress(int progress) {
        setProgress(progress, false);
    }

    synchronized void setProgress(int progress, boolean fromUser) {
        if (mIndeterminate) {
            return;
        }
        if (progress < 0) {
            progress = 0;
        }
        if (progress > mMax) {
            progress = mMax;
        }
        if (progress != mProgress) {
            mProgress = progress;
            refreshProgress(R.id.progress, mProgress, fromUser);
        }
    }

    @Override
    public void setProgressDrawable(Drawable d) {
        boolean needUpdate;
        if (mProgressDrawable != null && d != mProgressDrawable) {
            mProgressDrawable.setCallback(null);
            needUpdate = true;
        } else {
            needUpdate = false;
        }
        if (d != null) {
            d.setCallback(this);
            int drawableHeight = d.getMinimumHeight();
            if (mMaxHeight < drawableHeight) {
                mMaxHeight = drawableHeight;
                requestLayout();
            }
        }
        mProgressDrawable = d;
        if (!mIndeterminate) {
            mCurrentDrawable = d;
            postInvalidate();
        }
        if (needUpdate) {
            updateDrawableBounds(getWidth(), getHeight());
            updateDrawableState();
            doRefreshProgress(R.id.progress, mProgress, false, false);
            doRefreshProgress(R.id.secondaryProgress, mSecondaryProgress,
                    false, false);
        }
    }

    @Override
    public synchronized void setSecondaryProgress(int secondaryProgress) {
        if (mIndeterminate) {
            return;
        }
        if (secondaryProgress < 0) {
            secondaryProgress = 0;
        }
        if (secondaryProgress > mMax) {
            secondaryProgress = mMax;
        }
        if (secondaryProgress != mSecondaryProgress) {
            mSecondaryProgress = secondaryProgress;
            refreshProgress(R.id.secondaryProgress, mSecondaryProgress, false);
        }
    }

    @Override
    public void setVisibility(int v) {
        if (getVisibility() != v) {
            super.setVisibility(v);
            if (mIndeterminate) {
                if (v == android.view.View.GONE
                        || v == android.view.View.INVISIBLE) {
                    stopAnimation();
                } else {
                    startAnimation();
                }
            }
        }
    }

    void startAnimation() {
        if (getVisibility() != android.view.View.VISIBLE) {
            return;
        }
        if (mIndeterminateDrawable instanceof Animatable) {
            mShouldStartAnimationDrawable = true;
            mHasAnimation = false;
        } else {
            mHasAnimation = true;
            if (mInterpolator == null) {
                mInterpolator = new LinearInterpolator();
            }
            if (mTransformation == null) {
                mTransformation = new Transformation();
            } else {
                mTransformation.clear();
            }
            if (mAnimation == null) {
                mAnimation = new AlphaAnimation(0.0f, 1.0f);
            } else {
                mAnimation.reset();
            }
            mAnimation.setRepeatMode(mBehavior);
            mAnimation.setRepeatCount(Animation.INFINITE);
            mAnimation.setDuration(mDuration);
            mAnimation.setInterpolator(mInterpolator);
            mAnimation.setStartTime(Animation.START_ON_FIRST_FRAME);
        }
        postInvalidate();
    }

    void stopAnimation() {
        mHasAnimation = false;
        if (mIndeterminateDrawable instanceof Animatable) {
            ((Animatable) mIndeterminateDrawable).stop();
            mShouldStartAnimationDrawable = false;
        }
        postInvalidate();
    }

    private Drawable tileify(Drawable drawable, boolean clip) {
        if (drawable instanceof LayerDrawable) {
            LayerDrawable background = (LayerDrawable) drawable;
            final int N = background.getNumberOfLayers();
            Drawable[] outDrawables = new Drawable[N];
            for (int i = 0; i < N; i++) {
                int id = background.getId(i);
                outDrawables[i] = tileify(background.getDrawable(i),
                        id == R.id.progress || id == R.id.secondaryProgress);
            }
            LayerDrawable newBg = new LayerDrawable(outDrawables);
            for (int i = 0; i < N; i++) {
                newBg.setId(i, background.getId(i));
            }
            return newBg;
        } else if (drawable instanceof StateListDrawable) {
            StateListDrawable in = (StateListDrawable) drawable;
            StateListDrawable out = new StateListDrawable();
            int numStates = ReflectHelper
                    .invoke(in, "getStateCount", int.class);
            for (int i = 0; i < numStates; i++) {
                out.addState(
                        ReflectHelper.invoke(in, "getStateSet", int[].class, i),
                        tileify(ReflectHelper.invoke(in, "getStateDrawable",
                                Drawable.class, i), clip));
            }
            return out;
        } else if (drawable instanceof BitmapDrawable) {
            final Bitmap tileBitmap = ((BitmapDrawable) drawable).getBitmap();
            if (mSampleTile == null) {
                mSampleTile = tileBitmap;
            }
            final ShapeDrawable shapeDrawable = new ShapeDrawable(
                    getDrawableShape());
            final BitmapShader bitmapShader = new BitmapShader(tileBitmap,
                    Shader.TileMode.REPEAT, Shader.TileMode.CLAMP);
            shapeDrawable.getPaint().setShader(bitmapShader);
            return clip ? new ClipDrawable(shapeDrawable, Gravity.LEFT,
                    ClipDrawable.HORIZONTAL) : shapeDrawable;
        }
        return drawable;
    }

    private Drawable tileifyIndeterminate(Drawable drawable) {
        if (drawable instanceof AnimationDrawable) {
            AnimationDrawable background = (AnimationDrawable) drawable;
            final int N = background.getNumberOfFrames();
            AnimationDrawable newBg = new AnimationDrawable();
            newBg.setOneShot(background.isOneShot());
            for (int i = 0; i < N; i++) {
                Drawable frame = tileify(background.getFrame(i), true);
                frame.setLevel(10000);
                newBg.addFrame(frame, background.getDuration(i));
            }
            newBg.setLevel(10000);
            drawable = newBg;
        }
        return drawable;
    }

    private void updateDrawableBounds(int w, int h) {
        int right = w - getPaddingRight() - getPaddingLeft();
        int bottom = h - getPaddingBottom() - getPaddingTop();
        int top = 0;
        int left = 0;
        if (mIndeterminateDrawable != null) {
            if (mOnlyIndeterminate
                    && !(mIndeterminateDrawable instanceof AnimationDrawable)) {
                final int intrinsicWidth = mIndeterminateDrawable
                        .getIntrinsicWidth();
                final int intrinsicHeight = mIndeterminateDrawable
                        .getIntrinsicHeight();
                final float intrinsicAspect = (float) intrinsicWidth
                        / intrinsicHeight;
                final float boundAspect = (float) w / h;
                if (intrinsicAspect != boundAspect) {
                    if (boundAspect > intrinsicAspect) {
                        final int width = (int) (h * intrinsicAspect);
                        left = (w - width) / 2;
                        right = left + width;
                    } else {
                        final int height = (int) (w * (1 / intrinsicAspect));
                        top = (h - height) / 2;
                        bottom = top + height;
                    }
                }
            }
            mIndeterminateDrawable.setBounds(left, top, right, bottom);
        }
        if (mProgressDrawable != null) {
            mProgressDrawable.setBounds(0, 0, right, bottom);
        }
    }

    private void updateDrawableState() {
        int[] state = getDrawableState();
        if (mProgressDrawable != null && mProgressDrawable.isStateful()) {
            mProgressDrawable.setState(state);
        }
        if (mIndeterminateDrawable != null
                && mIndeterminateDrawable.isStateful()) {
            mIndeterminateDrawable.setState(state);
        }
    }

    @Override
    protected boolean verifyDrawable(Drawable who) {
        return who == mProgressDrawable || who == mIndeterminateDrawable
                || super.verifyDrawable(who);
    }
}
