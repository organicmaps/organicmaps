
package org.holoeverywhere.drawable;

import android.content.res.Resources;
import android.graphics.Canvas;
import android.graphics.ColorFilter;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.os.SystemClock;

public class DrawableContainer extends Drawable implements Drawable.Callback {
    public abstract static class DrawableContainerState extends ConstantState {
        boolean mCanConstantState;
        int mChangingConfigurations;
        boolean mCheckedConstantState;
        int mChildrenChangingConfigurations;
        boolean mComputedConstantSize = false;
        int mConstantHeight;
        int mConstantMinimumHeight;
        int mConstantMinimumWidth;
        Rect mConstantPadding = null;
        boolean mConstantSize = false;
        int mConstantWidth;
        boolean mDither = DEFAULT_DITHER;
        Drawable[] mDrawables;
        int mEnterFadeDuration;
        int mExitFadeDuration;
        boolean mHaveOpacity = false;
        boolean mHaveStateful = false;
        int mNumChildren;
        int mOpacity;
        final DrawableContainer mOwner;
        boolean mPaddingChecked = false;
        boolean mStateful;
        boolean mVariablePadding = false;

        DrawableContainerState(DrawableContainerState orig, DrawableContainer owner,
                Resources res) {
            mOwner = owner;
            if (orig != null) {
                mChangingConfigurations = orig.mChangingConfigurations;
                mChildrenChangingConfigurations = orig.mChildrenChangingConfigurations;
                final Drawable[] origDr = orig.mDrawables;
                mDrawables = new Drawable[origDr.length];
                mNumChildren = orig.mNumChildren;
                final int N = mNumChildren;
                for (int i = 0; i < N; i++) {
                    if (res != null) {
                        mDrawables[i] = origDr[i].getConstantState().newDrawable(res);
                    } else {
                        mDrawables[i] = origDr[i].getConstantState().newDrawable();
                    }
                    mDrawables[i].setCallback(owner);
                }
                mCheckedConstantState = mCanConstantState = true;
                mVariablePadding = orig.mVariablePadding;
                if (orig.mConstantPadding != null) {
                    mConstantPadding = new Rect(orig.mConstantPadding);
                }
                mConstantSize = orig.mConstantSize;
                mComputedConstantSize = orig.mComputedConstantSize;
                mConstantWidth = orig.mConstantWidth;
                mConstantHeight = orig.mConstantHeight;
                mHaveOpacity = orig.mHaveOpacity;
                mOpacity = orig.mOpacity;
                mHaveStateful = orig.mHaveStateful;
                mStateful = orig.mStateful;
                mDither = orig.mDither;
                mEnterFadeDuration = orig.mEnterFadeDuration;
                mExitFadeDuration = orig.mExitFadeDuration;
            } else {
                mDrawables = new Drawable[10];
                mNumChildren = 0;
                mCheckedConstantState = mCanConstantState = false;
            }
        }

        public final int addChild(Drawable dr) {
            final int pos = mNumChildren;
            if (pos >= mDrawables.length) {
                growArray(pos, pos + 10);
            }
            dr.setVisible(false, true);
            dr.setCallback(mOwner);
            mDrawables[pos] = dr;
            mNumChildren++;
            mChildrenChangingConfigurations |= dr.getChangingConfigurations();
            mHaveOpacity = false;
            mHaveStateful = false;
            mConstantPadding = null;
            mPaddingChecked = false;
            mComputedConstantSize = false;
            return pos;
        }

        public synchronized boolean canConstantState() {
            if (!mCheckedConstantState) {
                mCanConstantState = true;
                final int N = mNumChildren;
                for (int i = 0; i < N; i++) {
                    if (mDrawables[i].getConstantState() == null) {
                        mCanConstantState = false;
                        break;
                    }
                }
                mCheckedConstantState = true;
            }
            return mCanConstantState;
        }

        protected void computeConstantSize() {
            mComputedConstantSize = true;
            final int N = getChildCount();
            final Drawable[] drawables = mDrawables;
            mConstantWidth = mConstantHeight = -1;
            mConstantMinimumWidth = mConstantMinimumHeight = 0;
            for (int i = 0; i < N; i++) {
                Drawable dr = drawables[i];
                int s = dr.getIntrinsicWidth();
                if (s > mConstantWidth) {
                    mConstantWidth = s;
                }
                s = dr.getIntrinsicHeight();
                if (s > mConstantHeight) {
                    mConstantHeight = s;
                }
                s = dr.getMinimumWidth();
                if (s > mConstantMinimumWidth) {
                    mConstantMinimumWidth = s;
                }
                s = dr.getMinimumHeight();
                if (s > mConstantMinimumHeight) {
                    mConstantMinimumHeight = s;
                }
            }
        }

        @Override
        public int getChangingConfigurations() {
            return mChangingConfigurations;
        }

        public final int getChildCount() {
            return mNumChildren;
        }

        public final Drawable[] getChildren() {
            return mDrawables;
        }

        public final int getConstantHeight() {
            if (!mComputedConstantSize) {
                computeConstantSize();
            }

            return mConstantHeight;
        }

        public final int getConstantMinimumHeight() {
            if (!mComputedConstantSize) {
                computeConstantSize();
            }

            return mConstantMinimumHeight;
        }

        public final int getConstantMinimumWidth() {
            if (!mComputedConstantSize) {
                computeConstantSize();
            }

            return mConstantMinimumWidth;
        }

        public final Rect getConstantPadding() {
            if (mVariablePadding) {
                return null;
            }
            if (mConstantPadding != null || mPaddingChecked) {
                return mConstantPadding;
            }
            Rect r = null;
            final Rect t = new Rect();
            final int N = getChildCount();
            final Drawable[] drawables = mDrawables;
            for (int i = 0; i < N; i++) {
                if (drawables[i].getPadding(t)) {
                    if (r == null) {
                        r = new Rect(0, 0, 0, 0);
                    }
                    if (t.left > r.left) {
                        r.left = t.left;
                    }
                    if (t.top > r.top) {
                        r.top = t.top;
                    }
                    if (t.right > r.right) {
                        r.right = t.right;
                    }
                    if (t.bottom > r.bottom) {
                        r.bottom = t.bottom;
                    }
                }
            }
            mPaddingChecked = true;
            return mConstantPadding = r;
        }

        public final int getConstantWidth() {
            if (!mComputedConstantSize) {
                computeConstantSize();
            }
            return mConstantWidth;
        }

        public final int getEnterFadeDuration() {
            return mEnterFadeDuration;
        }

        public final int getExitFadeDuration() {
            return mExitFadeDuration;
        }

        public final int getOpacity() {
            if (mHaveOpacity) {
                return mOpacity;
            }
            final int N = getChildCount();
            final Drawable[] drawables = mDrawables;
            int op = N > 0 ? drawables[0].getOpacity() : PixelFormat.TRANSPARENT;
            for (int i = 1; i < N; i++) {
                op = Drawable.resolveOpacity(op, drawables[i].getOpacity());
            }
            mOpacity = op;
            mHaveOpacity = true;
            return op;
        }

        public void growArray(int oldSize, int newSize) {
            Drawable[] newDrawables = new Drawable[newSize];
            System.arraycopy(mDrawables, 0, newDrawables, 0, oldSize);
            mDrawables = newDrawables;
        }

        public final boolean isConstantSize() {
            return mConstantSize;
        }

        public final boolean isStateful() {
            if (mHaveStateful) {
                return mStateful;
            }
            boolean stateful = false;
            final int N = getChildCount();
            for (int i = 0; i < N; i++) {
                if (mDrawables[i].isStateful()) {
                    stateful = true;
                    break;
                }
            }
            mStateful = stateful;
            mHaveStateful = true;
            return stateful;
        }

        public final void setConstantSize(boolean constant) {
            mConstantSize = constant;
        }

        public final void setEnterFadeDuration(int duration) {
            mEnterFadeDuration = duration;
        }

        public final void setExitFadeDuration(int duration) {
            mExitFadeDuration = duration;
        }

        public final void setVariablePadding(boolean variable) {
            mVariablePadding = variable;
        }
    }

    private static final boolean DEFAULT_DITHER = true;
    private int mAlpha = 0xFF;
    private Runnable mAnimationRunnable;
    private ColorFilter mColorFilter;
    private int mCurIndex = -1;
    private Drawable mCurrDrawable;
    private DrawableContainerState mDrawableContainerState;
    private long mEnterAnimationEnd;
    private long mExitAnimationEnd;
    private Drawable mLastDrawable;
    private boolean mMutated;

    void animate(boolean schedule) {
        final long now = SystemClock.uptimeMillis();
        boolean animating = false;
        if (mCurrDrawable != null) {
            if (mEnterAnimationEnd != 0) {
                if (mEnterAnimationEnd <= now) {
                    mCurrDrawable.setAlpha(mAlpha);
                    mEnterAnimationEnd = 0;
                } else {
                    int animAlpha = (int) ((mEnterAnimationEnd - now) * 255)
                            / mDrawableContainerState.mEnterFadeDuration;
                    mCurrDrawable.setAlpha((255 - animAlpha) * mAlpha / 255);
                    animating = true;
                }
            }
        } else {
            mEnterAnimationEnd = 0;
        }
        if (mLastDrawable != null) {
            if (mExitAnimationEnd != 0) {
                if (mExitAnimationEnd <= now) {
                    mLastDrawable.setVisible(false, false);
                    mLastDrawable = null;
                    mExitAnimationEnd = 0;
                } else {
                    int animAlpha = (int) ((mExitAnimationEnd - now) * 255)
                            / mDrawableContainerState.mExitFadeDuration;
                    mLastDrawable.setAlpha(animAlpha * mAlpha / 255);
                    animating = true;
                }
            }
        } else {
            mExitAnimationEnd = 0;
        }
        if (schedule && animating) {
            scheduleSelf(mAnimationRunnable, now + 1000 / 60);
        }
    }

    @Override
    public void draw(Canvas canvas) {
        if (mCurrDrawable != null) {
            mCurrDrawable.draw(canvas);
        }
        if (mLastDrawable != null) {
            mLastDrawable.draw(canvas);
        }
    }

    @Override
    public int getChangingConfigurations() {
        return super.getChangingConfigurations()
                | mDrawableContainerState.mChangingConfigurations
                | mDrawableContainerState.mChildrenChangingConfigurations;
    }

    @Override
    public ConstantState getConstantState() {
        if (mDrawableContainerState.canConstantState()) {
            mDrawableContainerState.mChangingConfigurations = getChangingConfigurations();
            return mDrawableContainerState;
        }
        return null;
    }

    @Override
    public Drawable getCurrent() {
        return mCurrDrawable;
    }

    @Override
    public int getIntrinsicHeight() {
        if (mDrawableContainerState.isConstantSize()) {
            return mDrawableContainerState.getConstantHeight();
        }
        return mCurrDrawable != null ? mCurrDrawable.getIntrinsicHeight() : -1;
    }

    @Override
    public int getIntrinsicWidth() {
        if (mDrawableContainerState.isConstantSize()) {
            return mDrawableContainerState.getConstantWidth();
        }
        return mCurrDrawable != null ? mCurrDrawable.getIntrinsicWidth() : -1;
    }

    @Override
    public int getMinimumHeight() {
        if (mDrawableContainerState.isConstantSize()) {
            return mDrawableContainerState.getConstantMinimumHeight();
        }
        return mCurrDrawable != null ? mCurrDrawable.getMinimumHeight() : 0;
    }

    @Override
    public int getMinimumWidth() {
        if (mDrawableContainerState.isConstantSize()) {
            return mDrawableContainerState.getConstantMinimumWidth();
        }
        return mCurrDrawable != null ? mCurrDrawable.getMinimumWidth() : 0;
    }

    @Override
    public int getOpacity() {
        return mCurrDrawable == null || !mCurrDrawable.isVisible() ? PixelFormat.TRANSPARENT :
                mDrawableContainerState.getOpacity();
    }

    @Override
    public boolean getPadding(Rect padding) {
        final Rect r = mDrawableContainerState.getConstantPadding();
        if (r != null) {
            padding.set(r);
            return true;
        }
        if (mCurrDrawable != null) {
            return mCurrDrawable.getPadding(padding);
        } else {
            return super.getPadding(padding);
        }
    }

    @Override
    public void invalidateDrawable(Drawable who) {
        if (who == mCurrDrawable && getCallback() != null) {
            getCallback().invalidateDrawable(this);
        }
    }

    @Override
    public boolean isStateful() {
        return mDrawableContainerState.isStateful();
    }

    @Override
    public void jumpToCurrentState() {
        boolean changed = false;
        if (mLastDrawable != null) {
            mLastDrawable.jumpToCurrentState();
            mLastDrawable = null;
            changed = true;
        }
        if (mCurrDrawable != null) {
            mCurrDrawable.jumpToCurrentState();
            mCurrDrawable.setAlpha(mAlpha);
        }
        if (mExitAnimationEnd != 0) {
            mExitAnimationEnd = 0;
            changed = true;
        }
        if (mEnterAnimationEnd != 0) {
            mEnterAnimationEnd = 0;
            changed = true;
        }
        if (changed) {
            invalidateSelf();
        }
    }

    @Override
    public Drawable mutate() {
        if (!mMutated && super.mutate() == this) {
            final int N = mDrawableContainerState.getChildCount();
            final Drawable[] drawables = mDrawableContainerState.getChildren();
            for (int i = 0; i < N; i++) {
                if (drawables[i] != null) {
                    drawables[i].mutate();
                }
            }
            mMutated = true;
        }
        return this;
    }

    @Override
    protected void onBoundsChange(Rect bounds) {
        if (mLastDrawable != null) {
            mLastDrawable.setBounds(bounds);
        }
        if (mCurrDrawable != null) {
            mCurrDrawable.setBounds(bounds);
        }
    }

    @Override
    protected boolean onLevelChange(int level) {
        if (mLastDrawable != null) {
            return mLastDrawable.setLevel(level);
        }
        if (mCurrDrawable != null) {
            return mCurrDrawable.setLevel(level);
        }
        return false;
    }

    @Override
    protected boolean onStateChange(int[] state) {
        if (mLastDrawable != null) {
            return mLastDrawable.setState(state);
        }
        if (mCurrDrawable != null) {
            return mCurrDrawable.setState(state);
        }
        return false;
    }

    @Override
    public void scheduleDrawable(Drawable who, Runnable what, long when) {
        if (who == mCurrDrawable && getCallback() != null) {
            getCallback().scheduleDrawable(this, what, when);
        }
    }

    public boolean selectDrawable(int idx) {
        if (idx == mCurIndex) {
            return false;
        }
        final long now = SystemClock.uptimeMillis();
        if (mDrawableContainerState.mExitFadeDuration > 0) {
            if (mLastDrawable != null) {
                mLastDrawable.setVisible(false, false);
            }
            if (mCurrDrawable != null) {
                mLastDrawable = mCurrDrawable;
                mExitAnimationEnd = now + mDrawableContainerState.mExitFadeDuration;
            } else {
                mLastDrawable = null;
                mExitAnimationEnd = 0;
            }
        } else if (mCurrDrawable != null) {
            mCurrDrawable.setVisible(false, false);
        }

        if (idx >= 0 && idx < mDrawableContainerState.mNumChildren) {
            Drawable d = mDrawableContainerState.mDrawables[idx];
            mCurrDrawable = d;
            mCurIndex = idx;
            if (d != null) {
                if (mDrawableContainerState.mEnterFadeDuration > 0) {
                    mEnterAnimationEnd = now + mDrawableContainerState.mEnterFadeDuration;
                } else {
                    d.setAlpha(mAlpha);
                }
                d.setVisible(isVisible(), true);
                d.setDither(mDrawableContainerState.mDither);
                d.setColorFilter(mColorFilter);
                d.setState(getState());
                d.setLevel(getLevel());
                d.setBounds(getBounds());
            }
        } else {
            mCurrDrawable = null;
            mCurIndex = -1;
        }

        if (mEnterAnimationEnd != 0 || mExitAnimationEnd != 0) {
            if (mAnimationRunnable == null) {
                mAnimationRunnable = new Runnable() {
                    @Override
                    public void run() {
                        animate(true);
                        invalidateSelf();
                    }
                };
            } else {
                unscheduleSelf(mAnimationRunnable);
            }
            animate(true);
        }

        invalidateSelf();

        return true;
    }

    /**
     * TODO
     *
     * @Override public Insets getLayoutInsets() { return (mCurrDrawable ==
     *           null) ? Insets.NONE : mCurrDrawable.getLayoutInsets(); }
     */

    @Override
    public void setAlpha(int alpha) {
        if (mAlpha != alpha) {
            mAlpha = alpha;
            if (mCurrDrawable != null) {
                if (mEnterAnimationEnd == 0) {
                    mCurrDrawable.setAlpha(alpha);
                } else {
                    animate(false);
                }
            }
        }
    }

    @Override
    public void setColorFilter(ColorFilter cf) {
        if (mColorFilter != cf) {
            mColorFilter = cf;
            if (mCurrDrawable != null) {
                mCurrDrawable.setColorFilter(cf);
            }
        }
    }

    protected void setConstantState(DrawableContainerState state)
    {
        mDrawableContainerState = state;
    }

    @Override
    public void setDither(boolean dither) {
        if (mDrawableContainerState.mDither != dither) {
            mDrawableContainerState.mDither = dither;
            if (mCurrDrawable != null) {
                mCurrDrawable.setDither(mDrawableContainerState.mDither);
            }
        }
    }

    public void setEnterFadeDuration(int ms) {
        mDrawableContainerState.mEnterFadeDuration = ms;
    }

    public void setExitFadeDuration(int ms) {
        mDrawableContainerState.mExitFadeDuration = ms;
    }

    @Override
    public boolean setVisible(boolean visible, boolean restart) {
        boolean changed = super.setVisible(visible, restart);
        if (mLastDrawable != null) {
            mLastDrawable.setVisible(visible, restart);
        }
        if (mCurrDrawable != null) {
            mCurrDrawable.setVisible(visible, restart);
        }
        return changed;
    }

    @Override
    public void unscheduleDrawable(Drawable who, Runnable what) {
        if (who == mCurrDrawable && getCallback() != null) {
            getCallback().unscheduleDrawable(this, what);
        }
    }
}
