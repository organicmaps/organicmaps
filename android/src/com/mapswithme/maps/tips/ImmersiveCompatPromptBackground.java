package com.mapswithme.maps.tips;

import android.content.res.Resources;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.PointF;
import android.graphics.Rect;
import android.graphics.RectF;
import android.support.annotation.ColorInt;
import android.support.annotation.NonNull;
import android.util.DisplayMetrics;

import uk.co.samuelwall.materialtaptargetprompt.extras.PromptBackground;
import uk.co.samuelwall.materialtaptargetprompt.extras.PromptOptions;
import uk.co.samuelwall.materialtaptargetprompt.extras.PromptUtils;

/**
 * {@link PromptBackground} implementation that renders the prompt background as a rectangle.
 */
public class ImmersiveCompatPromptBackground extends PromptBackground
{
    private RectF mBounds, mBaseBounds;
    private Paint mPaint;
    private int mBaseColourAlpha;
    private PointF mFocalCentre;
    int mHeight = 0;

    /**
     * Constructor.
     */
    public ImmersiveCompatPromptBackground()
    {
        mPaint = new Paint();
        mPaint.setAntiAlias(true);
        mBounds = new RectF();
        mBaseBounds = new RectF();
        mFocalCentre = new PointF();
    }

    @Override
    public void setColour(@ColorInt int colour)
    {
        mPaint.setColor(colour);
        mBaseColourAlpha = Color.alpha(colour);
        mPaint.setAlpha(mBaseColourAlpha);
    }

    public void setHeight(int height)
    {
        mHeight = height;
    }

    @Override
    public void prepare(@NonNull final PromptOptions options, final boolean clipToBounds, @NonNull Rect clipBounds)
    {
        final RectF focalBounds = options.getPromptFocal().getBounds();
        DisplayMetrics metrics = Resources.getSystem().getDisplayMetrics();

        mBaseBounds.set(0, 0, metrics.widthPixels, mHeight == 0 ? metrics.heightPixels : mHeight);
        mFocalCentre.x = focalBounds.centerX();
        mFocalCentre.y = focalBounds.centerY();
    }

    @Override
    public void update(@NonNull final PromptOptions prompt, float revealModifier,
                       float alphaModifier)
    {
        mPaint.setAlpha((int) (mBaseColourAlpha * alphaModifier));
        PromptUtils.scale(mFocalCentre, mBaseBounds, mBounds, revealModifier, false);
    }

    @Override
    public void draw(@NonNull Canvas canvas)
    {
        canvas.drawRect(mBounds, mPaint);
    }

    @Override
    public boolean contains(float x, float y)
    {
        return mBounds.contains(x, y);
    }
}
