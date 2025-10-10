package app.organicmaps.sdk.widget.roadshield;

import android.graphics.Canvas;
import android.graphics.ColorFilter;
import android.graphics.Paint;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.graphics.Typeface;
import android.graphics.drawable.Drawable;
import androidx.annotation.ColorInt;
import androidx.annotation.NonNull;
import app.organicmaps.sdk.routing.roadshield.RoadShield;
import app.organicmaps.sdk.routing.roadshield.RoadShieldType;
import java.util.Map;
import java.util.Objects;

public class RoadShieldDrawable extends Drawable
{
  private interface ColorConstants
  {
    @ColorInt
    int WHITE = 0xFFFFFFFF;

    @ColorInt
    int BLACK = 0xFF000000;

    @ColorInt
    int YELLOW = 0xFFFFD400;

    @ColorInt
    int BLUE = 0xFF1A5EC1;

    @ColorInt
    int GREEN = 0xFF309302;

    @ColorInt
    int RED = 0xFFE63534;

    @ColorInt
    int ORANGE = 0xFFFFBE00;
  }

  // clang-format off
  private static final Map<RoadShieldType, Integer> SHIELD_TEXT_COLORS = Map.of(
    RoadShieldType.GenericWhite, ColorConstants.BLACK,
    RoadShieldType.GenericGreen, ColorConstants.WHITE,
    RoadShieldType.GenericBlue, ColorConstants.WHITE,
    RoadShieldType.GenericRed, ColorConstants.WHITE,
    RoadShieldType.GenericOrange, ColorConstants.BLACK,
    RoadShieldType.USInterstate, ColorConstants.WHITE,
    RoadShieldType.USHighway, ColorConstants.BLACK,
    RoadShieldType.UKHighway, ColorConstants.YELLOW
  );
  // clang-format on

  // clang-format off
  private static final Map<RoadShieldType, Integer> SHIELD_BACKGROUND_COLORS = Map.of(
      RoadShieldType.GenericWhite, ColorConstants.WHITE,
      RoadShieldType.GenericGreen, ColorConstants.GREEN,
      RoadShieldType.GenericBlue, ColorConstants.BLUE,
      RoadShieldType.GenericRed, ColorConstants.RED,
      RoadShieldType.GenericOrange, ColorConstants.ORANGE,
      RoadShieldType.USInterstate, ColorConstants.BLUE,
      RoadShieldType.USHighway, ColorConstants.WHITE,
      RoadShieldType.UKHighway, ColorConstants.GREEN
  );
  // clang-format on

  private final RoadShield mShield;
  private final boolean mDrawOutline;

  private final Paint mTextPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
  private final Paint mBorderPaint = new Paint(Paint.ANTI_ALIAS_FLAG);

  private int mWidth;
  private int mHeight;
  private float mOutlineWidth;

  public RoadShieldDrawable(@NonNull RoadShield shield, float textSize, boolean drawOutline)
  {
    mShield = shield;
    mDrawOutline = drawOutline;
    configureTextPaint(textSize);
    calculateDimensions();
    configureBackgroundPaint();
  }

  public void draw(@NonNull Canvas canvas)
  {
    // Draw shield background.
    if (!mDrawOutline)
      canvas.drawRoundRect(0 + mOutlineWidth, 0 + mOutlineWidth, mWidth - mOutlineWidth, mHeight - mOutlineWidth,
                           mHeight / 5.0f, mHeight / 5.0f, mBorderPaint);
    else
    {
      canvas.drawRoundRect(0, 0, mWidth, mHeight, mHeight / 5.0f, mHeight / 5.0f, mBorderPaint);
      @ColorInt
      final int oldBorderColor = mBorderPaint.getColor();
      mBorderPaint.setStyle(Paint.Style.STROKE);
      mBorderPaint.setColor(mTextPaint.getColor());
      canvas.drawRoundRect(0, 0, mWidth, mHeight, mHeight / 5.0f, mHeight / 5.0f, mBorderPaint);
      mBorderPaint.setStyle(Paint.Style.FILL);
      mBorderPaint.setColor(oldBorderColor);
    }

    // Draw shield text.
    final Rect textBounds = new Rect();
    mTextPaint.getTextBounds(mShield.text(), 0, mShield.text().length(), textBounds);
    final float centerX = mWidth / 2.0f;
    final float centerY = mHeight / 2.0f;
    final float textY = centerY - textBounds.exactCenterY();
    canvas.drawText(mShield.text(), centerX, textY, mTextPaint);
  }

  @Override
  public int getIntrinsicWidth()
  {
    return mWidth;
  }

  @Override
  public int getIntrinsicHeight()
  {
    return mHeight;
  }

  @Override
  public int getOpacity()
  {
    return PixelFormat.UNKNOWN;
  }

  @Override
  public void setAlpha(int alpha)
  {}

  @Override
  public void setColorFilter(ColorFilter colorFilter)
  {}

  private void configureTextPaint(float textSize)
  {
    final int textColor = Objects.requireNonNull(SHIELD_TEXT_COLORS.get(mShield.type()));

    mTextPaint.setColor(textColor);
    mTextPaint.setTextAlign(Paint.Align.CENTER);
    mTextPaint.setStyle(Paint.Style.FILL);
    mTextPaint.setTextSize(textSize);
    mTextPaint.setTypeface(Typeface.create(Typeface.DEFAULT, Typeface.BOLD));
  }

  private void configureBackgroundPaint()
  {
    final int backgroundColor = Objects.requireNonNull(SHIELD_BACKGROUND_COLORS.get(mShield.type()));

    mBorderPaint.setColor(backgroundColor);
    mBorderPaint.setStyle(Paint.Style.FILL);
    mBorderPaint.setStrokeWidth(mWidth * 0.03f);
  }

  private void calculateDimensions()
  {
    final Rect bounds = new Rect();
    mTextPaint.getTextBounds(mShield.text(), 0, mShield.text().length(), bounds);
    mWidth = (int) (bounds.width() + mTextPaint.getTextSize() * 1.5f);
    mHeight = (int) (bounds.height() + mTextPaint.getTextSize() * 0.6f);
    mOutlineWidth = mHeight / 10.0f;
    setBounds(0, 0, getIntrinsicWidth(), getIntrinsicHeight());
  }
}
