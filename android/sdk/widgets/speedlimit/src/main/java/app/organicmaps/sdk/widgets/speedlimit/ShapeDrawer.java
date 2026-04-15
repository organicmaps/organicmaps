package app.organicmaps.sdk.widgets.speedlimit;

import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.Typeface;
import android.view.MotionEvent;
import androidx.annotation.NonNull;
import androidx.annotation.RestrictTo;

@RestrictTo(RestrictTo.Scope.LIBRARY)
abstract class ShapeDrawer
{
  private static final String FILLER_2 = "88";
  private static final String FILLER_3 = "888";

  @NonNull
  protected final Paint mBackgroundPaint;
  @NonNull
  protected final Paint mBorderPaint;
  @NonNull
  protected final Paint mTextPaint;

  @NonNull
  protected SpeedLimitView.Colors mColors = new SpeedLimitView.Colors(0, 0, 0);

  protected float mWidth;
  protected float mHeight;

  protected float mBorderWidthRatio;
  protected float mCornerRadiusRatio;

  @NonNull
  protected String mValue = "0";

  public ShapeDrawer(float borderWidthRatio, float cornerRadiusRatio)
  {
    mBackgroundPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    mBorderPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    mBorderPaint.setStyle(Paint.Style.STROKE);
    mTextPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    mTextPaint.setTextAlign(Paint.Align.CENTER);
    mTextPaint.setTypeface(Typeface.create(Typeface.DEFAULT, Typeface.BOLD));
    mBorderWidthRatio = borderWidthRatio;
    mCornerRadiusRatio = cornerRadiusRatio;
  }

  public void setValue(@NonNull String value)
  {
    mValue = value;
    configureTextSize();
  }

  public void setColors(@NonNull SpeedLimitView.Colors colors)
  {
    mColors = colors;
  }

  public abstract void setSize(float width, float height);

  public abstract boolean onTouchEvent(@NonNull MotionEvent event);

  public final void onDraw(@NonNull Canvas canvas)
  {
    drawSign(canvas);
    drawText(canvas, mWidth / 2, mHeight / 2);
  }

  protected abstract void drawSign(@NonNull Canvas canvas);

  protected abstract void onConfigureTextSize(@NonNull String string);

  protected void configureTextSize()
  {
    onConfigureTextSize(mValue.length() < 3 ? FILLER_2 : FILLER_3);
  }

  protected final void drawText(@NonNull Canvas canvas, float cx, float cy)
  {
    mTextPaint.setColor(mColors.textColor());

    final Rect textBounds = new Rect();
    mTextPaint.getTextBounds(mValue, 0, mValue.length(), textBounds);
    final float textY = cy - textBounds.exactCenterY();
    canvas.drawText(mValue, cx, textY, mTextPaint);
  }

  // Apply binary search to determine the optimal text size satisfying the given fit condition.
  protected final void configureTextSizeImpl(@NonNull String text, float maxSize, @NonNull TextFitPredicate fits)
  {
    if (maxSize <= 0)
    {
      mTextPaint.setTextSize(1);
      return;
    }

    float lowerBound = 0;
    float upperBound = maxSize;
    float textSize = maxSize;
    final Rect textBounds = new Rect();

    while (lowerBound <= upperBound)
    {
      textSize = (lowerBound + upperBound) / 2;
      mTextPaint.setTextSize(textSize);
      mTextPaint.getTextBounds(text, 0, text.length(), textBounds);

      if (fits.fits(new RectF(textBounds)))
        lowerBound = textSize + 1;
      else
        upperBound = textSize - 1;
    }

    mTextPaint.setTextSize(Math.max(1, textSize));
  }
}
