package app.organicmaps.sdk.widgets.speedlimit;

import android.graphics.Canvas;
import android.graphics.RectF;
import android.view.MotionEvent;
import androidx.annotation.NonNull;
import androidx.annotation.RestrictTo;

@RestrictTo(RestrictTo.Scope.LIBRARY)
final class SquareShapeDrawer extends ShapeDrawer
{
  private float mBorderWidth;
  private float mCornerRadius;
  private final RectF mBackgroundRect = new RectF();
  private final RectF mBorderRect = new RectF();

  public SquareShapeDrawer(float borderWidthRatio, float cornerRadiusRatio)
  {
    super(borderWidthRatio, cornerRadiusRatio);
  }

  @Override
  public void setSize(float width, float height)
  {
    mWidth = width;
    mHeight = height;
    final float minSide = Math.min(mWidth, mHeight);
    mBorderWidth = minSide * mBorderWidthRatio;
    mCornerRadius = minSide * mCornerRadiusRatio;
    final float left = (mWidth - minSide) / 2;
    final float top = (mHeight - minSide) / 2;
    mBackgroundRect.set(left, top, left + minSide, top + minSide);
    final float halfBorder = mBorderWidth / 2;
    mBorderRect.set(left + halfBorder, top + halfBorder, left + minSide - halfBorder, top + minSide - halfBorder);
    configureTextSize();
  }

  @Override
  public boolean onTouchEvent(@NonNull MotionEvent event)
  {
    return mBackgroundRect.contains(event.getX(), event.getY());
  }

  @Override
  protected void drawSign(@NonNull Canvas canvas)
  {
    mBackgroundPaint.setColor(mColors.backgroundColor());
    canvas.drawRoundRect(mBackgroundRect, mCornerRadius, mCornerRadius, mBackgroundPaint);

    mBorderPaint.setColor(mColors.borderColor());
    mBorderPaint.setStrokeWidth(mBorderWidth);
    final float borderCornerRadius = Math.max(0, mCornerRadius - mBorderWidth / 2);
    canvas.drawRoundRect(mBorderRect, borderCornerRadius, borderCornerRadius, mBorderPaint);
  }

  @Override
  protected void onConfigureTextSize(@NonNull String text)
  {
    final float inner = Math.min(mWidth, mHeight) - mBorderWidth * 3.5f;
    configureTextSizeImpl(text, inner, bounds -> bounds.width() <= inner && bounds.height() <= inner);
  }
}
