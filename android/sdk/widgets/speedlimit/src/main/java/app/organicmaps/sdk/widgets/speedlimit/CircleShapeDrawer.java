package app.organicmaps.sdk.widgets.speedlimit;

import android.graphics.Canvas;
import android.view.MotionEvent;
import androidx.annotation.NonNull;
import androidx.annotation.RestrictTo;

@RestrictTo(RestrictTo.Scope.LIBRARY)
final class CircleShapeDrawer extends ShapeDrawer
{
  private float mBackgroundRadius;
  private float mBorderRadius;
  private float mBorderWidth;

  public CircleShapeDrawer(float borderWidthRatio, float cornerRadiusRatio)
  {
    super(borderWidthRatio, cornerRadiusRatio);
  }

  @Override
  public void setSize(float width, float height)
  {
    mWidth = width;
    mHeight = height;
    mBackgroundRadius = Math.min(mWidth, mHeight) / 2;
    mBorderWidth = mBackgroundRadius * 2 * mBorderWidthRatio;
    mBorderRadius = mBackgroundRadius - mBorderWidth / 2;
    configureTextSize();
  }

  @Override
  public boolean onTouchEvent(@NonNull MotionEvent event)
  {
    final float cx = mWidth / 2;
    final float cy = mHeight / 2;
    return Math.pow(event.getX() - cx, 2) + Math.pow(event.getY() - cy, 2) <= Math.pow(mBackgroundRadius, 2);
  }

  @Override
  protected void drawSign(@NonNull Canvas canvas)
  {
    final float cx = mWidth / 2;
    final float cy = mHeight / 2;

    mBackgroundPaint.setColor(mColors.backgroundColor());
    canvas.drawCircle(cx, cy, mBackgroundRadius, mBackgroundPaint);

    mBorderPaint.setColor(mColors.borderColor());
    mBorderPaint.setStrokeWidth(mBorderWidth);
    canvas.drawCircle(cx, cy, mBorderRadius, mBorderPaint);
  }

  @Override
  protected void onConfigureTextSize(@NonNull String text)
  {
    final float textRadius = mBorderRadius - mBorderWidth;
    final float textMaxSize = 2 * textRadius;
    final float textMaxSizeSquared = (float) Math.pow(textMaxSize, 2);
    configureTextSizeImpl(text, textMaxSize,
                          bounds -> Math.pow(bounds.width(), 2) + Math.pow(bounds.height(), 2) <= textMaxSizeSquared);
  }
}
