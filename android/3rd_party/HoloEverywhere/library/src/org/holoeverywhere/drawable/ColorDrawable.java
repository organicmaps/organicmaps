
package org.holoeverywhere.drawable;

import java.io.IOException;

import org.holoeverywhere.R;
import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;

import android.content.res.Resources;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.ColorFilter;
import android.graphics.Paint;
import android.graphics.PixelFormat;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;

public class ColorDrawable extends android.graphics.drawable.ColorDrawable {
    final static class ColorState extends ConstantState {
        int mBaseColor;
        int mChangingConfigurations;
        int mUseColor;

        ColorState(ColorState state) {
            if (state != null) {
                mBaseColor = state.mBaseColor;
                mUseColor = state.mUseColor;
            }
        }

        @Override
        public int getChangingConfigurations() {
            return mChangingConfigurations;
        }

        @Override
        public Drawable newDrawable() {
            return new ColorDrawable(this);
        }

        @Override
        public Drawable newDrawable(Resources res) {
            return new ColorDrawable(this);
        }
    }

    private final Paint mPaint = new Paint();
    private ColorState mState;

    public ColorDrawable() {
        this(null);
    }

    private ColorDrawable(ColorState state) {
        mState = new ColorState(state);
    }

    public ColorDrawable(int color) {
        this(null);
        setColor(color);
    }

    @Override
    public void draw(Canvas canvas) {
        if (mState.mUseColor >>> 24 != 0) {
            mPaint.setColor(mState.mUseColor);
            canvas.drawRect(getBounds(), mPaint);
        }
    }

    @Override
    public int getAlpha() {
        return mState.mUseColor >>> 24;
    }

    @Override
    public int getChangingConfigurations() {
        return super.getChangingConfigurations() | mState.mChangingConfigurations;
    }

    @Override
    public int getColor() {
        return mState.mUseColor;
    }

    @Override
    public ConstantState getConstantState() {
        mState.mChangingConfigurations = getChangingConfigurations();
        return mState;
    }

    @Override
    public int getOpacity() {
        switch (mState.mUseColor >>> 24) {
            case 255:
                return PixelFormat.OPAQUE;
            case 0:
                return PixelFormat.TRANSPARENT;
        }
        return PixelFormat.TRANSLUCENT;
    }

    @Override
    public void inflate(Resources r, XmlPullParser parser, AttributeSet attrs)
            throws XmlPullParserException, IOException {
        super.inflate(r, parser, attrs);
        TypedArray a = r.obtainAttributes(attrs, R.styleable.ColorDrawable);
        int color = mState.mBaseColor;
        color = a.getColor(R.styleable.ColorDrawable_android_color, color);
        mState.mBaseColor = mState.mUseColor = color;
        a.recycle();
    }

    @Override
    public void setAlpha(int alpha) {
        alpha += alpha >> 7;
        int baseAlpha = mState.mBaseColor >>> 24;
        int useAlpha = baseAlpha * alpha >> 8;
        int oldUseColor = mState.mUseColor;
        mState.mUseColor = mState.mBaseColor << 8 >>> 8 | useAlpha << 24;
        if (oldUseColor != mState.mUseColor) {
            invalidateSelf();
        }
    }

    @Override
    public void setColor(int color) {
        if (mState.mBaseColor != color || mState.mUseColor != color) {
            invalidateSelf();
            mState.mBaseColor = mState.mUseColor = color;
        }
    }

    @Override
    public void setColorFilter(ColorFilter colorFilter) {
    }
}
