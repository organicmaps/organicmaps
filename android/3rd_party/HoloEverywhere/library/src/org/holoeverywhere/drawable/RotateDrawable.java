
package org.holoeverywhere.drawable;

import java.io.IOException;

import org.holoeverywhere.R;
import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;

import android.content.res.Resources;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.ColorFilter;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.util.Log;
import android.util.TypedValue;

public class RotateDrawable extends Drawable implements Drawable.Callback {
    /**
     * <p>
     * Represents the state of a rotation for a given drawable. The same rotate
     * drawable can be invoked with different states to drive several rotations
     * at the same time.
     * </p>
     */
    final static class RotateState extends Drawable.ConstantState {
        private boolean mCanConstantState;

        int mChangingConfigurations;

        private boolean mCheckedConstantState;
        float mCurrentDegrees;
        Drawable mDrawable;
        float mFromDegrees;

        float mPivotX;
        boolean mPivotXRel;

        float mPivotY;

        boolean mPivotYRel;
        float mToDegrees;

        public RotateState(RotateState source, RotateDrawable owner, Resources res) {
            if (source != null) {
                if (res != null) {
                    mDrawable = source.mDrawable.getConstantState().newDrawable(res);
                } else {
                    mDrawable = source.mDrawable.getConstantState().newDrawable();
                }
                mDrawable.setCallback(owner);
                mPivotXRel = source.mPivotXRel;
                mPivotX = source.mPivotX;
                mPivotYRel = source.mPivotYRel;
                mPivotY = source.mPivotY;
                mFromDegrees = mCurrentDegrees = source.mFromDegrees;
                mToDegrees = source.mToDegrees;
                mCanConstantState = mCheckedConstantState = true;
            }
        }

        public boolean canConstantState() {
            if (!mCheckedConstantState) {
                mCanConstantState = mDrawable.getConstantState() != null;
                mCheckedConstantState = true;
            }

            return mCanConstantState;
        }

        @Override
        public int getChangingConfigurations() {
            return mChangingConfigurations;
        }

        @Override
        public Drawable newDrawable() {
            return new RotateDrawable(this, null);
        }

        @Override
        public Drawable newDrawable(Resources res) {
            return new RotateDrawable(this, res);
        }
    }

    private static final float MAX_LEVEL = 10000.0f;
    private boolean mMutated;

    private RotateState mState;

    public RotateDrawable() {
        this(null, null);
    }

    private RotateDrawable(RotateState rotateState, Resources res) {
        mState = new RotateState(rotateState, this, res);
    }

    @Override
    public void draw(Canvas canvas) {
        int saveCount = canvas.save();
        Rect bounds = mState.mDrawable.getBounds();
        int w = bounds.right - bounds.left;
        int h = bounds.bottom - bounds.top;
        final RotateState st = mState;
        float px = st.mPivotXRel ? w * st.mPivotX : st.mPivotX;
        float py = st.mPivotYRel ? h * st.mPivotY : st.mPivotY;
        canvas.rotate(st.mCurrentDegrees, px + bounds.left, py + bounds.top);
        st.mDrawable.draw(canvas);
        canvas.restoreToCount(saveCount);
    }

    @Override
    public int getChangingConfigurations() {
        return super.getChangingConfigurations()
                | mState.mChangingConfigurations
                | mState.mDrawable.getChangingConfigurations();
    }

    @Override
    public ConstantState getConstantState() {
        if (mState.canConstantState()) {
            mState.mChangingConfigurations = getChangingConfigurations();
            return mState;
        }
        return null;
    }

    public Drawable getDrawable() {
        return mState.mDrawable;
    }

    @Override
    public int getIntrinsicHeight() {
        return mState.mDrawable.getIntrinsicHeight();
    }

    @Override
    public int getIntrinsicWidth() {
        return mState.mDrawable.getIntrinsicWidth();
    }

    @Override
    public int getOpacity() {
        return mState.mDrawable.getOpacity();
    }

    @Override
    public boolean getPadding(Rect padding) {
        return mState.mDrawable.getPadding(padding);
    }

    @Override
    public void inflate(Resources r, XmlPullParser parser, AttributeSet attrs)
            throws XmlPullParserException, IOException {
        super.inflate(r, parser, attrs);
        TypedArray a = r.obtainAttributes(attrs,
                R.styleable.RotateDrawable);
        super.setVisible(a.getBoolean(R.styleable.RotateDrawable_android_visible, true), false);
        TypedValue tv = a.peekValue(R.styleable.RotateDrawable_android_pivotX);
        boolean pivotXRel;
        float pivotX;
        if (tv == null) {
            pivotXRel = true;
            pivotX = 0.5f;
        } else {
            pivotXRel = tv.type == TypedValue.TYPE_FRACTION;
            pivotX = pivotXRel ? tv.getFraction(1.0f, 1.0f) : tv.getFloat();
        }
        tv = a.peekValue(R.styleable.RotateDrawable_android_pivotY);
        boolean pivotYRel;
        float pivotY;
        if (tv == null) {
            pivotYRel = true;
            pivotY = 0.5f;
        } else {
            pivotYRel = tv.type == TypedValue.TYPE_FRACTION;
            pivotY = pivotYRel ? tv.getFraction(1.0f, 1.0f) : tv.getFloat();
        }
        float fromDegrees = a.getFloat(R.styleable.RotateDrawable_android_fromDegrees, 0.0f);
        float toDegrees = a.getFloat(R.styleable.RotateDrawable_android_toDegrees, 360.0f);
        int res = a.getResourceId(R.styleable.RotateDrawable_android_drawable, 0);
        Drawable drawable = null;
        if (res > 0) {
            drawable = r.getDrawable(res);
        }
        a.recycle();
        int outerDepth = parser.getDepth();
        int type;
        while ((type = parser.next()) != XmlPullParser.END_DOCUMENT &&
                (type != XmlPullParser.END_TAG || parser.getDepth() > outerDepth)) {

            if (type != XmlPullParser.START_TAG) {
                continue;
            }

            if ((drawable = Drawable.createFromXmlInner(r, parser, attrs)) == null) {
                Log.w("drawable", "Bad element under <rotate>: "
                        + parser.getName());
            }
        }

        if (drawable == null) {
            Log.w("drawable", "No drawable specified for <rotate>");
        }

        mState.mDrawable = drawable;
        mState.mPivotXRel = pivotXRel;
        mState.mPivotX = pivotX;
        mState.mPivotYRel = pivotYRel;
        mState.mPivotY = pivotY;
        mState.mFromDegrees = mState.mCurrentDegrees = fromDegrees;
        mState.mToDegrees = toDegrees;

        if (drawable != null) {
            drawable.setCallback(this);
        }
    }

    @Override
    public void invalidateDrawable(Drawable who) {
        final Callback callback = getCallback();
        if (callback != null) {
            callback.invalidateDrawable(this);
        }
    }

    @Override
    public boolean isStateful() {
        return mState.mDrawable.isStateful();
    }

    @Override
    public Drawable mutate() {
        if (!mMutated && super.mutate() == this) {
            mState.mDrawable.mutate();
            mMutated = true;
        }
        return this;
    }

    @Override
    protected void onBoundsChange(Rect bounds) {
        mState.mDrawable.setBounds(bounds.left, bounds.top,
                bounds.right, bounds.bottom);
    }

    @Override
    protected boolean onLevelChange(int level) {
        mState.mDrawable.setLevel(level);
        onBoundsChange(getBounds());
        mState.mCurrentDegrees = mState.mFromDegrees +
                (mState.mToDegrees - mState.mFromDegrees) *
                (level / MAX_LEVEL);
        invalidateSelf();
        return true;
    }

    @Override
    protected boolean onStateChange(int[] state) {
        boolean changed = mState.mDrawable.setState(state);
        onBoundsChange(getBounds());
        return changed;
    }

    @Override
    public void scheduleDrawable(Drawable who, Runnable what, long when) {
        final Callback callback = getCallback();
        if (callback != null) {
            callback.scheduleDrawable(this, what, when);
        }
    }

    @Override
    public void setAlpha(int alpha) {
        mState.mDrawable.setAlpha(alpha);
    }

    @Override
    public void setColorFilter(ColorFilter cf) {
        mState.mDrawable.setColorFilter(cf);
    }

    @Override
    public boolean setVisible(boolean visible, boolean restart) {
        mState.mDrawable.setVisible(visible, restart);
        return super.setVisible(visible, restart);
    }

    @Override
    public void unscheduleDrawable(Drawable who, Runnable what) {
        final Callback callback = getCallback();
        if (callback != null) {
            callback.unscheduleDrawable(this, what);
        }
    }
}
