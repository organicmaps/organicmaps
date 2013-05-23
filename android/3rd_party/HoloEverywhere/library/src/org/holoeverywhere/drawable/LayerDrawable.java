
package org.holoeverywhere.drawable;

import java.io.IOException;

import org.holoeverywhere.R;
import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;

import android.content.res.Resources;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.ColorFilter;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.os.Build.VERSION;
import android.os.Build.VERSION_CODES;
import android.util.AttributeSet;
import android.view.View;

public class LayerDrawable extends Drawable implements Drawable.Callback {
    static class ChildDrawable {
        public Drawable mDrawable;
        public int mId;
        public int mInsetL, mInsetT, mInsetR, mInsetB;
    }

    static class LayerState extends ConstantState {
        private boolean mCanConstantState;
        int mChangingConfigurations;
        private boolean mCheckedConstantState;
        ChildDrawable[] mChildren;
        int mChildrenChangingConfigurations;
        private boolean mHaveOpacity = false;
        private boolean mHaveStateful = false;
        int mNum;
        private int mOpacity;
        private boolean mStateful;

        LayerState(LayerState orig, LayerDrawable owner, Resources res) {
            if (orig != null) {
                final ChildDrawable[] origChildDrawable = orig.mChildren;
                final int N = orig.mNum;
                mNum = N;
                mChildren = new ChildDrawable[N];
                mChangingConfigurations = orig.mChangingConfigurations;
                mChildrenChangingConfigurations = orig.mChildrenChangingConfigurations;
                for (int i = 0; i < N; i++) {
                    final ChildDrawable r = mChildren[i] = new ChildDrawable();
                    final ChildDrawable or = origChildDrawable[i];
                    if (res != null) {
                        r.mDrawable = or.mDrawable.getConstantState().newDrawable(res);
                    } else {
                        r.mDrawable = or.mDrawable.getConstantState().newDrawable();
                    }
                    r.mDrawable.setCallback(owner);
                    r.mInsetL = or.mInsetL;
                    r.mInsetT = or.mInsetT;
                    r.mInsetR = or.mInsetR;
                    r.mInsetB = or.mInsetB;
                    r.mId = or.mId;
                }
                mHaveOpacity = orig.mHaveOpacity;
                mOpacity = orig.mOpacity;
                mHaveStateful = orig.mHaveStateful;
                mStateful = orig.mStateful;
                mCheckedConstantState = mCanConstantState = true;
            } else {
                mNum = 0;
                mChildren = null;
            }
        }

        public boolean canConstantState() {
            if (!mCheckedConstantState && mChildren != null) {
                mCanConstantState = true;
                final int N = mNum;
                for (int i = 0; i < N; i++) {
                    if (mChildren[i].mDrawable.getConstantState() == null) {
                        mCanConstantState = false;
                        break;
                    }
                }
                mCheckedConstantState = true;
            }
            return mCanConstantState;
        }

        @Override
        public int getChangingConfigurations() {
            return mChangingConfigurations;
        }

        public final int getOpacity() {
            if (mHaveOpacity) {
                return mOpacity;
            }
            final int N = mNum;
            int op = N > 0 ? mChildren[0].mDrawable.getOpacity() : PixelFormat.TRANSPARENT;
            for (int i = 1; i < N; i++) {
                op = Drawable.resolveOpacity(op, mChildren[i].mDrawable.getOpacity());
            }
            mOpacity = op;
            mHaveOpacity = true;
            return op;
        }

        public final boolean isStateful() {
            if (mHaveStateful) {
                return mStateful;
            }
            boolean stateful = false;
            final int N = mNum;
            for (int i = 0; i < N; i++) {
                if (mChildren[i].mDrawable.isStateful()) {
                    stateful = true;
                    break;
                }
            }
            mStateful = stateful;
            mHaveStateful = true;
            return stateful;
        }

        @Override
        public Drawable newDrawable() {
            return new LayerDrawable(this, null);
        }

        @Override
        public Drawable newDrawable(Resources res) {
            return new LayerDrawable(this, res);
        }
    }

    LayerState mLayerState;
    private boolean mMutated;
    private int mOpacityOverride = PixelFormat.UNKNOWN;
    private int[] mPaddingB;
    private int[] mPaddingL;
    private int[] mPaddingR;

    private int[] mPaddingT;

    private final Rect mTmpRect = new Rect();

    LayerDrawable() {
        this((LayerState) null, null);
    }

    public LayerDrawable(Drawable[] layers) {
        this(layers, null);
    }

    LayerDrawable(Drawable[] layers, LayerState state) {
        this(state, null);
        int length = layers.length;
        ChildDrawable[] r = new ChildDrawable[length];

        for (int i = 0; i < length; i++) {
            r[i] = new ChildDrawable();
            r[i].mDrawable = layers[i];
            layers[i].setCallback(this);
            mLayerState.mChildrenChangingConfigurations |= layers[i].getChangingConfigurations();
        }
        mLayerState.mNum = length;
        mLayerState.mChildren = r;

        ensurePadding();
    }

    LayerDrawable(LayerState state, Resources res) {
        LayerState as = createConstantState(state, res);
        mLayerState = as;
        if (as.mNum > 0) {
            ensurePadding();
        }
    }

    private void addLayer(Drawable layer, int id, int left, int top, int right, int bottom) {
        final LayerState st = mLayerState;
        int N = st.mChildren != null ? st.mChildren.length : 0;
        int i = st.mNum;
        if (i >= N) {
            ChildDrawable[] nu = new ChildDrawable[N + 10];
            if (i > 0) {
                System.arraycopy(st.mChildren, 0, nu, 0, i);
            }
            st.mChildren = nu;
        }
        mLayerState.mChildrenChangingConfigurations |= layer.getChangingConfigurations();
        ChildDrawable childDrawable = new ChildDrawable();
        st.mChildren[i] = childDrawable;
        childDrawable.mId = id;
        childDrawable.mDrawable = layer;
        childDrawable.mInsetL = left;
        childDrawable.mInsetT = top;
        childDrawable.mInsetR = right;
        childDrawable.mInsetB = bottom;
        st.mNum++;
        layer.setCallback(this);
    }

    LayerState createConstantState(LayerState state, Resources res) {
        return new LayerState(state, this, res);
    }

    @Override
    public void draw(Canvas canvas) {
        final ChildDrawable[] array = mLayerState.mChildren;
        final int N = mLayerState.mNum;
        for (int i = 0; i < N; i++) {
            array[i].mDrawable.draw(canvas);
        }
    }

    private void ensurePadding() {
        final int N = mLayerState.mNum;
        if (mPaddingL != null && mPaddingL.length >= N) {
            return;
        }
        mPaddingL = new int[N];
        mPaddingT = new int[N];
        mPaddingR = new int[N];
        mPaddingB = new int[N];
    }

    public Drawable findDrawableByLayerId(int id) {
        final ChildDrawable[] layers = mLayerState.mChildren;
        for (int i = mLayerState.mNum - 1; i >= 0; i--) {
            if (layers[i].mId == id) {
                return layers[i].mDrawable;
            }
        }
        return null;
    }

    @Override
    public Callback getCallback() {
        if (VERSION.SDK_INT >= VERSION_CODES.HONEYCOMB) {
            return super.getCallback();
        } else {
            return null;
        }
    }

    @Override
    public int getChangingConfigurations() {
        return super.getChangingConfigurations()
                | mLayerState.mChangingConfigurations
                | mLayerState.mChildrenChangingConfigurations;
    }

    @Override
    public ConstantState getConstantState() {
        if (mLayerState.canConstantState()) {
            mLayerState.mChangingConfigurations = getChangingConfigurations();
            return mLayerState;
        }
        return null;
    }

    public Drawable getDrawable(int index) {
        return mLayerState.mChildren[index].mDrawable;
    }

    public int getId(int index) {
        return mLayerState.mChildren[index].mId;
    }

    @Override
    public int getIntrinsicHeight() {
        int height = -1;
        final ChildDrawable[] array = mLayerState.mChildren;
        final int N = mLayerState.mNum;
        int padT = 0, padB = 0;
        for (int i = 0; i < N; i++) {
            final ChildDrawable r = array[i];
            int h = r.mDrawable.getIntrinsicHeight() + r.mInsetT + r.mInsetB + +padT + padB;
            if (h > height) {
                height = h;
            }
            padT += mPaddingT[i];
            padB += mPaddingB[i];
        }
        return height;
    }

    @Override
    public int getIntrinsicWidth() {
        int width = -1;
        final ChildDrawable[] array = mLayerState.mChildren;
        final int N = mLayerState.mNum;
        int padL = 0, padR = 0;
        for (int i = 0; i < N; i++) {
            final ChildDrawable r = array[i];
            int w = r.mDrawable.getIntrinsicWidth()
                    + r.mInsetL + r.mInsetR + padL + padR;
            if (w > width) {
                width = w;
            }
            padL += mPaddingL[i];
            padR += mPaddingR[i];
        }
        return width;
    }

    public int getNumberOfLayers() {
        return mLayerState.mNum;
    }

    @Override
    public int getOpacity() {
        if (mOpacityOverride != PixelFormat.UNKNOWN) {
            return mOpacityOverride;
        }
        return mLayerState.getOpacity();
    }

    @Override
    public boolean getPadding(Rect padding) {
        padding.left = 0;
        padding.top = 0;
        padding.right = 0;
        padding.bottom = 0;
        final ChildDrawable[] array = mLayerState.mChildren;
        final int N = mLayerState.mNum;
        for (int i = 0; i < N; i++) {
            reapplyPadding(i, array[i]);
            padding.left += mPaddingL[i];
            padding.top += mPaddingT[i];
            padding.right += mPaddingR[i];
            padding.bottom += mPaddingB[i];
        }
        return true;
    }

    @Override
    public void inflate(Resources r, XmlPullParser parser, AttributeSet attrs)
            throws XmlPullParserException, IOException {
        super.inflate(r, parser, attrs);
        int type;
        TypedArray a = r.obtainAttributes(attrs, R.styleable.LayerDrawable);
        mOpacityOverride = a.getInt(R.styleable.LayerDrawable_android_opacity,
                PixelFormat.UNKNOWN);
        a.recycle();
        final int innerDepth = parser.getDepth() + 1;
        int depth;
        while ((type = parser.next()) != XmlPullParser.END_DOCUMENT
                && ((depth = parser.getDepth()) >= innerDepth || type != XmlPullParser.END_TAG)) {
            if (type != XmlPullParser.START_TAG) {
                continue;
            }
            if (depth > innerDepth || !parser.getName().equals("item")) {
                continue;
            }
            a = r.obtainAttributes(attrs, R.styleable.LayerDrawableItem);
            int left = a.getDimensionPixelOffset(R.styleable.LayerDrawableItem_android_left, 0);
            int top = a.getDimensionPixelOffset(R.styleable.LayerDrawableItem_android_top, 0);
            int right = a.getDimensionPixelOffset(R.styleable.LayerDrawableItem_android_right, 0);
            int bottom = a.getDimensionPixelOffset(R.styleable.LayerDrawableItem_android_bottom, 0);
            int drawableRes = a.getResourceId(R.styleable.LayerDrawableItem_android_drawable, 0);
            int id = a.getResourceId(R.styleable.LayerDrawableItem_android_id, View.NO_ID);
            a.recycle();
            Drawable dr;
            if (drawableRes != 0) {
                dr = DrawableCompat.getDrawable(r, drawableRes);
            } else {
                while ((type = parser.next()) == XmlPullParser.TEXT) {
                }
                if (type != XmlPullParser.START_TAG) {
                    throw new XmlPullParserException(parser.getPositionDescription()
                            + ": <item> tag requires a 'drawable' attribute or "
                            + "child tag defining a drawable");
                }
                dr = DrawableCompat.createFromXmlInner(r, parser, attrs);
            }
            addLayer(dr, id, left, top, right, bottom);
        }
        ensurePadding();
        onStateChange(getState());
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
        return mLayerState.isStateful();
    }

    @Override
    public Drawable mutate() {
        if (!mMutated && super.mutate() == this) {
            if (!mLayerState.canConstantState()) {
                throw new IllegalStateException("One or more children of this LayerDrawable does " +
                        "not have constant state; this drawable cannot be mutated.");
            }
            mLayerState = new LayerState(mLayerState, this, null);
            final ChildDrawable[] array = mLayerState.mChildren;
            final int N = mLayerState.mNum;
            for (int i = 0; i < N; i++) {
                array[i].mDrawable.mutate();
            }
            mMutated = true;
        }
        return this;
    }

    @Override
    protected void onBoundsChange(Rect bounds) {
        final ChildDrawable[] array = mLayerState.mChildren;
        final int N = mLayerState.mNum;
        int padL = 0, padT = 0, padR = 0, padB = 0;
        for (int i = 0; i < N; i++) {
            final ChildDrawable r = array[i];
            r.mDrawable.setBounds(bounds.left + r.mInsetL + padL,
                    bounds.top + r.mInsetT + padT,
                    bounds.right - r.mInsetR - padR,
                    bounds.bottom - r.mInsetB - padB);
            padL += mPaddingL[i];
            padR += mPaddingR[i];
            padT += mPaddingT[i];
            padB += mPaddingB[i];
        }
    }

    @Override
    protected boolean onLevelChange(int level) {
        final ChildDrawable[] array = mLayerState.mChildren;
        final int N = mLayerState.mNum;
        boolean paddingChanged = false;
        boolean changed = false;
        for (int i = 0; i < N; i++) {
            final ChildDrawable r = array[i];
            if (r.mDrawable.setLevel(level)) {
                changed = true;
            }
            if (reapplyPadding(i, r)) {
                paddingChanged = true;
            }
        }
        if (paddingChanged) {
            onBoundsChange(getBounds());
        }
        return changed;
    }

    @Override
    protected boolean onStateChange(int[] state) {
        final ChildDrawable[] array = mLayerState.mChildren;
        final int N = mLayerState.mNum;
        boolean paddingChanged = false;
        boolean changed = false;
        for (int i = 0; i < N; i++) {
            final ChildDrawable r = array[i];
            if (r.mDrawable.setState(state)) {
                changed = true;
            }
            if (reapplyPadding(i, r)) {
                paddingChanged = true;
            }
        }
        if (paddingChanged) {
            onBoundsChange(getBounds());
        }
        if (changed) {
            invalidateSelf();
        }
        return changed;
    }

    private boolean reapplyPadding(int i, ChildDrawable r) {
        final Rect rect = mTmpRect;
        r.mDrawable.getPadding(rect);
        if (rect.left != mPaddingL[i] || rect.top != mPaddingT[i] ||
                rect.right != mPaddingR[i] || rect.bottom != mPaddingB[i]) {
            mPaddingL[i] = rect.left;
            mPaddingT[i] = rect.top;
            mPaddingR[i] = rect.right;
            mPaddingB[i] = rect.bottom;
            return true;
        }
        return false;
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
        final ChildDrawable[] array = mLayerState.mChildren;
        final int N = mLayerState.mNum;
        for (int i = 0; i < N; i++) {
            array[i].mDrawable.setAlpha(alpha);
        }
    }

    @Override
    public void setColorFilter(ColorFilter cf) {
        final ChildDrawable[] array = mLayerState.mChildren;
        final int N = mLayerState.mNum;
        for (int i = 0; i < N; i++) {
            array[i].mDrawable.setColorFilter(cf);
        }
    }

    @Override
    public void setDither(boolean dither) {
        final ChildDrawable[] array = mLayerState.mChildren;
        final int N = mLayerState.mNum;
        for (int i = 0; i < N; i++) {
            array[i].mDrawable.setDither(dither);
        }
    }

    public boolean setDrawableByLayerId(int id, Drawable drawable) {
        final ChildDrawable[] layers = mLayerState.mChildren;
        for (int i = mLayerState.mNum - 1; i >= 0; i--) {
            if (layers[i].mId == id) {
                if (layers[i].mDrawable != null) {
                    if (drawable != null) {
                        Rect bounds = layers[i].mDrawable.getBounds();
                        drawable.setBounds(bounds);
                    }
                    layers[i].mDrawable.setCallback(null);
                }
                if (drawable != null) {
                    drawable.setCallback(this);
                }
                layers[i].mDrawable = drawable;
                return true;
            }
        }

        return false;
    }

    public void setId(int index, int id) {
        mLayerState.mChildren[index].mId = id;
    }

    public void setLayerInset(int index, int l, int t, int r, int b) {
        ChildDrawable childDrawable = mLayerState.mChildren[index];
        childDrawable.mInsetL = l;
        childDrawable.mInsetT = t;
        childDrawable.mInsetR = r;
        childDrawable.mInsetB = b;
    }

    public void setOpacity(int opacity) {
        mOpacityOverride = opacity;
    }

    @Override
    public boolean setVisible(boolean visible, boolean restart) {
        boolean changed = super.setVisible(visible, restart);
        final ChildDrawable[] array = mLayerState.mChildren;
        final int N = mLayerState.mNum;
        for (int i = 0; i < N; i++) {
            array[i].mDrawable.setVisible(visible, restart);
        }
        return changed;
    }

    @Override
    public void unscheduleDrawable(Drawable who, Runnable what) {
        final Callback callback = getCallback();
        if (callback != null) {
            callback.unscheduleDrawable(this, what);
        }
    }
}
