package app.organicmaps.widget;

import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.Typeface;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;
import androidx.annotation.ColorInt;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.R;

public class SpeedLimitView extends View
{
  private interface DefaultValues
  {
    @ColorInt
    int BACKGROUND_COLOR = Color.WHITE;
    @ColorInt
    int BORDER_COLOR = Color.RED;
    @ColorInt
    int ALERT_COLOR = Color.RED;
    @ColorInt
    int TEXT_COLOR = Color.BLACK;
    @ColorInt
    int TEXT_ALERT_COLOR = Color.WHITE;

    float BORDER_WIDTH_RATIO = 0.1f;
  }

  @ColorInt
  private final int mBackgroundColor;

  @ColorInt
  private final int mBorderColor;

  @ColorInt
  private final int mAlertColor;

  @ColorInt
  private final int mTextColor;

  @ColorInt
  private final int mTextAlertColor;

  @NonNull
  private final Paint mSignBackgroundPaint;
  @NonNull
  private final Paint mSignBorderPaint;
  @NonNull
  private final Paint mTextPaint;

  private float mWidth;
  private float mHeight;
  private float mBackgroundRadius;
  private float mBorderRadius;
  private float mBorderWidth;

  private int mSpeedLimit = 0;
  @NonNull
  private String mSpeedLimitStr = "0";
  private boolean mAlert = false;

  public SpeedLimitView(Context context, @Nullable AttributeSet attrs)
  {
    super(context, attrs);

    try (TypedArray data = context.getTheme().obtainStyledAttributes(attrs, R.styleable.SpeedLimitView, 0, 0))
    {
      mBackgroundColor =
          data.getColor(R.styleable.SpeedLimitView_speedLimitBackgroundColor, DefaultValues.BACKGROUND_COLOR);
      mBorderColor = data.getColor(R.styleable.SpeedLimitView_speedLimitBorderColor, DefaultValues.BORDER_COLOR);
      mAlertColor = data.getColor(R.styleable.SpeedLimitView_speedLimitAlertColor, DefaultValues.ALERT_COLOR);
      mTextColor = data.getColor(R.styleable.SpeedLimitView_speedLimitTextColor, DefaultValues.TEXT_COLOR);
      mTextAlertColor =
          data.getColor(R.styleable.SpeedLimitView_speedLimitTextAlertColor, DefaultValues.TEXT_ALERT_COLOR);
      if (isInEditMode())
      {
        mSpeedLimit = data.getInt(R.styleable.SpeedLimitView_speedLimitEditModeSpeedLimit, 60);
        mSpeedLimitStr = Integer.toString(mSpeedLimit);
        mAlert = data.getBoolean(R.styleable.SpeedLimitView_speedLimitEditModeAlert, false);
      }
    }

    mSignBackgroundPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    mSignBackgroundPaint.setColor(mBackgroundColor);

    mSignBorderPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    mSignBorderPaint.setColor(mBorderColor);
    mSignBorderPaint.setStrokeWidth(mBorderWidth);
    mSignBorderPaint.setStyle(Paint.Style.STROKE);

    mTextPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    mTextPaint.setColor(mTextColor);
    mTextPaint.setTextAlign(Paint.Align.CENTER);
    mTextPaint.setTypeface(Typeface.create(Typeface.DEFAULT, Typeface.BOLD));
  }

  public void setSpeedLimit(final int speedLimit, boolean alert)
  {
    final boolean speedLimitChanged = mSpeedLimit != speedLimit;

    mSpeedLimit = speedLimit;
    mAlert = alert;

    if (speedLimitChanged)
    {
      mSpeedLimitStr = Integer.toString(mSpeedLimit);
      configureTextSize();
    }

    invalidate();
  }

  @Override
  protected void onDraw(@NonNull Canvas canvas)
  {
    super.onDraw(canvas);

    final boolean validSpeedLimit = mSpeedLimit > 0;
    if (!validSpeedLimit)
      return;

    final float cx = mWidth / 2;
    final float cy = mHeight / 2;

    drawSign(canvas, cx, cy, mAlert);
    drawText(canvas, cx, cy, mAlert);
  }

  private void drawSign(@NonNull Canvas canvas, float cx, float cy, boolean alert)
  {
    if (alert)
      mSignBackgroundPaint.setColor(mAlertColor);
    else
      mSignBackgroundPaint.setColor(mBackgroundColor);

    canvas.drawCircle(cx, cy, mBackgroundRadius, mSignBackgroundPaint);
    if (!alert)
    {
      mSignBorderPaint.setStrokeWidth(mBorderWidth);
      canvas.drawCircle(cx, cy, mBorderRadius, mSignBorderPaint);
    }
  }

  private void drawText(@NonNull Canvas canvas, float cx, float cy, boolean alert)
  {
    if (alert)
      mTextPaint.setColor(mTextAlertColor);
    else
      mTextPaint.setColor(mTextColor);

    final Rect textBounds = new Rect();
    mTextPaint.getTextBounds(mSpeedLimitStr, 0, mSpeedLimitStr.length(), textBounds);
    final float textY = cy - textBounds.exactCenterY();
    canvas.drawText(mSpeedLimitStr, cx, textY, mTextPaint);
  }

  @Override
  public boolean onTouchEvent(@NonNull MotionEvent event)
  {
    final float cx = mWidth / 2;
    final float cy = mHeight / 2;
    if (Math.pow(event.getX() - cx, 2) + Math.pow(event.getY() - cy, 2) <= Math.pow(mBackgroundRadius, 2))
    {
      performClick();
      return true;
    }
    return false;
  }

  @Override
  public boolean performClick()
  {
    super.performClick();
    return false;
  }

  @Override
  protected void onSizeChanged(int w, int h, int oldw, int oldh)
  {
    super.onSizeChanged(w, h, oldw, oldh);

    final float paddingX = (float) (getPaddingLeft() + getPaddingRight());
    final float paddingY = (float) (getPaddingTop() + getPaddingBottom());

    mWidth = (float) w - paddingX;
    mHeight = (float) h - paddingY;
    mBackgroundRadius = Math.min(mWidth, mHeight) / 2;
    mBorderWidth = mBackgroundRadius * 2 * DefaultValues.BORDER_WIDTH_RATIO;
    mBorderRadius = mBackgroundRadius - mBorderWidth / 2;
    configureTextSize();
  }

  // Apply binary search to determine the optimal text size that fits within the circular boundary.
  private void configureTextSize()
  {
    final String text = mSpeedLimitStr;
    final float textRadius = mBorderRadius - mBorderWidth;
    final float textMaxSize = 2 * textRadius;
    final float textMaxSizeSquared = (float) Math.pow(textMaxSize, 2);

    float lowerBound = 0;
    float upperBound = textMaxSize;
    float textSize = textMaxSize;
    final Rect textBounds = new Rect();

    while (lowerBound <= upperBound)
    {
      textSize = (lowerBound + upperBound) / 2;
      mTextPaint.setTextSize(textSize);
      mTextPaint.getTextBounds(text, 0, text.length(), textBounds);

      if (Math.pow(textBounds.width(), 2) + Math.pow(textBounds.height(), 2) <= textMaxSizeSquared)
        lowerBound = textSize + 1;
      else
        upperBound = textSize - 1;
    }

    mTextPaint.setTextSize(Math.max(1, textSize));
  }
}
