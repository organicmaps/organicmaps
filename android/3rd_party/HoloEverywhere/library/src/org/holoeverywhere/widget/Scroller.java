
package org.holoeverywhere.widget;

import android.content.Context;
import android.hardware.SensorManager;
import android.os.Build;
import android.util.FloatMath;
import android.view.ViewConfiguration;
import android.view.animation.AnimationUtils;
import android.view.animation.Interpolator;

public class Scroller {
    private static float ALPHA = 800;
    private static float DECELERATION_RATE = (float) (Math.log(0.75) / Math
            .log(0.9));
    private static final int DEFAULT_DURATION = 250;
    private static float END_TENSION = 0.6f;
    private static final int FLING_MODE = 1;
    private static final int NB_SAMPLES = 100;
    private static final int SCROLL_MODE = 0;
    private static final float[] SPLINE = new float[Scroller.NB_SAMPLES + 1];
    private static float START_TENSION = 0.4f;
    private static float sViscousFluidNormalize;
    private static float sViscousFluidScale;
    static {
        float x_min = 0.0f;
        for (int i = 0; i <= Scroller.NB_SAMPLES; i++) {
            final float t = (float) i / Scroller.NB_SAMPLES;
            float x_max = 1.0f;
            float x, tx, coef;
            while (true) {
                x = x_min + (x_max - x_min) / 2.0f;
                coef = 3.0f * x * (1.0f - x);
                tx = coef
                        * ((1.0f - x) * Scroller.START_TENSION + x
                                * Scroller.END_TENSION) + x * x * x;
                if (Math.abs(tx - t) < 1E-5) {
                    break;
                }
                if (tx > t) {
                    x_max = x;
                } else {
                    x_min = x;
                }
            }
            final float d = coef + x * x * x;
            Scroller.SPLINE[i] = d;
        }
        Scroller.SPLINE[Scroller.NB_SAMPLES] = 1.0f;
        Scroller.sViscousFluidScale = 8.0f;
        Scroller.sViscousFluidNormalize = 1.0f;
        Scroller.sViscousFluidNormalize = 1.0f / Scroller.viscousFluid(1.0f);
    }

    static float viscousFluid(float x) {
        x *= Scroller.sViscousFluidScale;
        if (x < 1.0f) {
            x -= 1.0f - (float) Math.exp(-x);
        } else {
            float start = 0.36787944117f; // 1/e == exp(-1)
            x = 1.0f - (float) Math.exp(1.0f - x);
            x = start + x * (1.0f - start);
        }
        x *= Scroller.sViscousFluidNormalize;
        return x;
    }

    private int mCurrX;
    private int mCurrY;
    private float mDeceleration;
    private float mDeltaX;
    private float mDeltaY;
    private int mDuration;
    private float mDurationReciprocal;
    private int mFinalX;
    private int mFinalY;
    private boolean mFinished;
    private boolean mFlywheel;
    private Interpolator mInterpolator;
    private int mMaxX;
    private int mMaxY;
    private int mMinX;
    private int mMinY;
    private int mMode;
    private final float mPpi;

    private long mStartTime;

    private int mStartX;
    private int mStartY;

    private float mVelocity;

    public Scroller(Context context) {
        this(context, null);
    }

    public Scroller(Context context, Interpolator interpolator) {
        this(
                context,
                interpolator,
                context.getApplicationInfo().targetSdkVersion >= Build.VERSION_CODES.HONEYCOMB);
    }

    public Scroller(Context context, Interpolator interpolator, boolean flywheel) {
        mFinished = true;
        mInterpolator = interpolator;
        mPpi = context.getResources().getDisplayMetrics().density * 160.0f;
        mDeceleration = computeDeceleration(ViewConfiguration
                .getScrollFriction());
        mFlywheel = flywheel;
    }

    public void abortAnimation() {
        mCurrX = mFinalX;
        mCurrY = mFinalY;
        mFinished = true;
    }

    private float computeDeceleration(float friction) {
        return SensorManager.GRAVITY_EARTH * 39.37f * mPpi * friction;
    }

    public boolean computeScrollOffset() {
        if (mFinished) {
            return false;
        }
        int timePassed = (int) (AnimationUtils.currentAnimationTimeMillis() - mStartTime);
        if (timePassed < mDuration) {
            switch (mMode) {
                case SCROLL_MODE:
                    float x = timePassed * mDurationReciprocal;
                    if (mInterpolator == null) {
                        x = Scroller.viscousFluid(x);
                    } else {
                        x = mInterpolator.getInterpolation(x);
                    }
                    mCurrX = mStartX + Math.round(x * mDeltaX);
                    mCurrY = mStartY + Math.round(x * mDeltaY);
                    break;
                case FLING_MODE:
                    final float t = (float) timePassed / mDuration;
                    final int index = (int) (Scroller.NB_SAMPLES * t);
                    final float t_inf = (float) index / Scroller.NB_SAMPLES;
                    final float t_sup = (float) (index + 1) / Scroller.NB_SAMPLES;
                    final float d_inf = Scroller.SPLINE[index];
                    final float d_sup = Scroller.SPLINE[index + 1];
                    final float distanceCoef = d_inf + (t - t_inf)
                            / (t_sup - t_inf) * (d_sup - d_inf);
                    mCurrX = mStartX
                            + Math.round(distanceCoef * (mFinalX - mStartX));
                    mCurrX = Math.min(mCurrX, mMaxX);
                    mCurrX = Math.max(mCurrX, mMinX);
                    mCurrY = mStartY
                            + Math.round(distanceCoef * (mFinalY - mStartY));
                    mCurrY = Math.min(mCurrY, mMaxY);
                    mCurrY = Math.max(mCurrY, mMinY);
                    if (mCurrX == mFinalX && mCurrY == mFinalY) {
                        mFinished = true;
                    }
                    break;
            }
        } else {
            mCurrX = mFinalX;
            mCurrY = mFinalY;
            mFinished = true;
        }
        return true;
    }

    public void extendDuration(int extend) {
        int passed = timePassed();
        mDuration = passed + extend;
        mDurationReciprocal = 1.0f / mDuration;
        mFinished = false;
    }

    public void fling(int startX, int startY, int velocityX, int velocityY,
            int minX, int maxX, int minY, int maxY) {
        if (mFlywheel && !mFinished) {
            float oldVel = getCurrVelocity();
            float dx = mFinalX - mStartX;
            float dy = mFinalY - mStartY;
            float hyp = FloatMath.sqrt(dx * dx + dy * dy);
            float ndx = dx / hyp;
            float ndy = dy / hyp;
            float oldVelocityX = ndx * oldVel;
            float oldVelocityY = ndy * oldVel;
            if (Math.signum(velocityX) == Math.signum(oldVelocityX)
                    && Math.signum(velocityY) == Math.signum(oldVelocityY)) {
                velocityX += oldVelocityX;
                velocityY += oldVelocityY;
            }
        }
        mMode = Scroller.FLING_MODE;
        mFinished = false;
        float velocity = FloatMath.sqrt(velocityX * velocityX + velocityY
                * velocityY);
        mVelocity = velocity;
        final double l = Math.log(Scroller.START_TENSION * velocity
                / Scroller.ALPHA);
        mDuration = (int) (1000.0 * Math.exp(l
                / (Scroller.DECELERATION_RATE - 1.0)));
        mStartTime = AnimationUtils.currentAnimationTimeMillis();
        mStartX = startX;
        mStartY = startY;
        float coeffX = velocity == 0 ? 1.0f : velocityX / velocity;
        float coeffY = velocity == 0 ? 1.0f : velocityY / velocity;
        int totalDistance = (int) (Scroller.ALPHA * Math
                .exp(Scroller.DECELERATION_RATE
                        / (Scroller.DECELERATION_RATE - 1.0) * l));
        mMinX = minX;
        mMaxX = maxX;
        mMinY = minY;
        mMaxY = maxY;
        mFinalX = startX + Math.round(totalDistance * coeffX);
        mFinalX = Math.min(mFinalX, mMaxX);
        mFinalX = Math.max(mFinalX, mMinX);
        mFinalY = startY + Math.round(totalDistance * coeffY);
        mFinalY = Math.min(mFinalY, mMaxY);
        mFinalY = Math.max(mFinalY, mMinY);
    }

    public final void forceFinished(boolean finished) {
        mFinished = finished;
    }

    public float getCurrVelocity() {
        return mVelocity - mDeceleration * timePassed() / 2000.0f;
    }

    public final int getCurrX() {
        return mCurrX;
    }

    public final int getCurrY() {
        return mCurrY;
    }

    public final int getDuration() {
        return mDuration;
    }

    public final int getFinalX() {
        return mFinalX;
    }

    public final int getFinalY() {
        return mFinalY;
    }

    public final int getStartX() {
        return mStartX;
    }

    public final int getStartY() {
        return mStartY;
    }

    public final boolean isFinished() {
        return mFinished;
    }

    public boolean isScrollingInDirection(float xvel, float yvel) {
        return !mFinished
                && Math.signum(xvel) == Math.signum(mFinalX - mStartX)
                && Math.signum(yvel) == Math.signum(mFinalY - mStartY);
    }

    public void setFinalX(int newX) {
        mFinalX = newX;
        mDeltaX = mFinalX - mStartX;
        mFinished = false;
    }

    public void setFinalY(int newY) {
        mFinalY = newY;
        mDeltaY = mFinalY - mStartY;
        mFinished = false;
    }

    public final void setFriction(float friction) {
        mDeceleration = computeDeceleration(friction);
    }

    public void startScroll(int startX, int startY, int dx, int dy) {
        startScroll(startX, startY, dx, dy, Scroller.DEFAULT_DURATION);
    }

    public void startScroll(int startX, int startY, int dx, int dy, int duration) {
        mMode = Scroller.SCROLL_MODE;
        mFinished = false;
        mDuration = duration;
        mStartTime = AnimationUtils.currentAnimationTimeMillis();
        mStartX = startX;
        mStartY = startY;
        mFinalX = startX + dx;
        mFinalY = startY + dy;
        mDeltaX = dx;
        mDeltaY = dy;
        mDurationReciprocal = 1.0f / mDuration;
    }

    public int timePassed() {
        return (int) (AnimationUtils.currentAnimationTimeMillis() - mStartTime);
    }
}
