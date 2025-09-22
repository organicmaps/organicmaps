package app.organicmaps.sdk.widgets.roadsign.drawers;

import android.graphics.Canvas;
import android.graphics.Paint;
import androidx.annotation.NonNull;
import androidx.annotation.RestrictTo;
import androidx.annotation.RestrictTo.Scope;

@RestrictTo(Scope.LIBRARY)
public final class CircleBackgroundDrawer extends BackgroundDrawer
{
  private static class DrawingParams
  {
    float backgroundRadius = 0f;
    float outlineRadius = 0f;
    float edgeRadius = 0f;

    float edgeWidth = 0f;
    float outlineWidth = 0f;
  }

  @NonNull
  private final DrawingParams mDrawingParams = new DrawingParams();

  @Override
  public boolean onTouch(float x, float y)
  {
    final float center = mSize / 2.0f;
    return Math.pow(x - center, 2) + Math.pow(y - center, 2) <= Math.pow(center, 2);
  }

  @Override
  public void draw(@NonNull Canvas canvas, float cx, float cy)
  {
    mPaint.setColor(mConfig.backgroundColor);
    mPaint.setStyle(Paint.Style.FILL);
    canvas.drawCircle(cx, cy, mDrawingParams.backgroundRadius, mPaint);

    if (mDrawingParams.outlineWidth > 0)
    {
      mPaint.setColor(mConfig.outlineColor);
      mPaint.setStyle(Paint.Style.STROKE);
      mPaint.setStrokeWidth(mDrawingParams.outlineWidth);
      canvas.drawCircle(cx, cy, mDrawingParams.outlineRadius, mPaint);
    }

    if (mDrawingParams.edgeWidth > 0)
    {
      mPaint.setColor(mConfig.edgeColor);
      mPaint.setStyle(Paint.Style.STROKE);
      mPaint.setStrokeWidth(mDrawingParams.edgeWidth);
      canvas.drawCircle(cx, cy, mDrawingParams.edgeRadius, mPaint);
    }
  }

  @Override
  public float getInnerRadius()
  {
    return mDrawingParams.backgroundRadius * 0.9f;
  }

  @Override
  protected void updateDrawingParams()
  {
    mDrawingParams.edgeWidth = mSize * mConfig.edgeWidthRatio;
    mDrawingParams.outlineWidth = mSize * mConfig.outlineWidthRatio;
    final float backgroundWidth = mSize - mDrawingParams.outlineWidth * 2 - mDrawingParams.edgeWidth * 2;

    mDrawingParams.backgroundRadius = backgroundWidth / 2;
    mDrawingParams.outlineRadius = mDrawingParams.backgroundRadius + mDrawingParams.outlineWidth / 2;
    mDrawingParams.edgeRadius = (float) mSize / 2 - mDrawingParams.edgeWidth / 2;
  }
}
