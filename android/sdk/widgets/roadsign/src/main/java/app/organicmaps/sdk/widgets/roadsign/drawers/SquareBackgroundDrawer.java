package app.organicmaps.sdk.widgets.roadsign.drawers;

import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.RectF;
import androidx.annotation.NonNull;
import androidx.annotation.RestrictTo;
import androidx.annotation.RestrictTo.Scope;

@RestrictTo(Scope.LIBRARY)
public final class SquareBackgroundDrawer extends BackgroundDrawer
{
  private static class DrawingParams
  {
    final RectF backgroundRect = new RectF();
    final RectF outlineRect = new RectF();
    final RectF edgeRect = new RectF();

    float backgroundRadius = 0f;
    float outlineRadius = 0f;
    float edgeRadius = 0f;
  }

  @NonNull
  private final DrawingParams mDrawingParams = new DrawingParams();

  public SquareBackgroundDrawer()
  {
    super();
    mPaint.setStyle(Paint.Style.FILL);
  }

  @Override
  public boolean onTouch(float x, float y)
  {
    return x >= 0 && x <= mSize && y >= 0 && y <= mSize;
  }

  @Override
  public void draw(@NonNull Canvas canvas, float cx, float cy)
  {
    if (mConfig.edgeWidthRatio > 0)
    {
      mPaint.setColor(mConfig.edgeColor);
      canvas.drawRoundRect(mDrawingParams.edgeRect, mDrawingParams.edgeRadius, mDrawingParams.edgeRadius, mPaint);
    }

    if (mConfig.outlineWidthRatio > 0)
    {
      mPaint.setColor(mConfig.outlineColor);
      canvas.drawRoundRect(mDrawingParams.outlineRect, mDrawingParams.outlineRadius, mDrawingParams.outlineRadius,
                           mPaint);
    }

    mPaint.setColor(mConfig.backgroundColor);
    canvas.drawRoundRect(mDrawingParams.backgroundRect, mDrawingParams.backgroundRadius,
                         mDrawingParams.backgroundRadius, mPaint);
  }

  @Override
  public float getInnerRadius()
  {
    return mDrawingParams.backgroundRect.width() / 2 * 0.9f;
  }

  @Override
  protected void updateDrawingParams()
  {
    final float edgeSize = mSize * mConfig.edgeWidthRatio;
    final float outlineSize = mSize * mConfig.outlineWidthRatio;

    mDrawingParams.edgeRect.set(0, 0, mSize, mSize);
    mDrawingParams.edgeRadius = mDrawingParams.edgeRect.width() * mConfig.cornerRadiusRatio;

    mDrawingParams.outlineRect.set(edgeSize, edgeSize, mSize - edgeSize, mSize - edgeSize);
    mDrawingParams.outlineRadius = mDrawingParams.outlineRect.width() * mConfig.cornerRadiusRatio;

    mDrawingParams.backgroundRect.set(edgeSize + outlineSize, edgeSize + outlineSize, mSize - edgeSize - outlineSize,
                                      mSize - edgeSize - outlineSize);
    mDrawingParams.backgroundRadius = mDrawingParams.backgroundRect.width() * mConfig.cornerRadiusRatio;
  }
}
