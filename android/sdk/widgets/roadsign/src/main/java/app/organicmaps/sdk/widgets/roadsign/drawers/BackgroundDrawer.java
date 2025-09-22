package app.organicmaps.sdk.widgets.roadsign.drawers;

import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import androidx.annotation.ColorInt;
import androidx.annotation.FloatRange;
import androidx.annotation.NonNull;
import androidx.annotation.RestrictTo;
import androidx.annotation.RestrictTo.Scope;

@RestrictTo(Scope.LIBRARY)
public abstract sealed class BackgroundDrawer permits CircleBackgroundDrawer, SquareBackgroundDrawer
{
  public static class Config
  {
    @FloatRange(from = 0.0, to = 0.5)
    public float edgeWidthRatio = 0.02f;
    @FloatRange(from = 0.0, to = 0.5)
    public float outlineWidthRatio = 0.1f;
    @FloatRange(from = 0.0, to = 0.5)
    public float cornerRadiusRatio = 0.1f;

    @ColorInt
    public int edgeColor = Color.WHITE;
    @ColorInt
    public int outlineColor = Color.RED;
    @ColorInt
    public int backgroundColor = Color.WHITE;
  }

  @NonNull
  protected Config mConfig = new Config();

  @NonNull
  protected final Paint mPaint = new Paint(Paint.ANTI_ALIAS_FLAG);

  protected int mSize;

  public abstract boolean onTouch(float x, float y);

  public abstract void draw(@NonNull Canvas canvas, float cx, float cy);

  public abstract float getInnerRadius();

  protected abstract void updateDrawingParams();

  public final void setColors(@ColorInt int backgroundColor, @ColorInt int outlineColor, @ColorInt int edgeColor)
  {
    mConfig.backgroundColor = backgroundColor;
    mConfig.outlineColor = outlineColor;
    mConfig.edgeColor = edgeColor;
  }

  public final void setConfig(@NonNull Config config)
  {
    mConfig = config;
    updateDrawingParams();
  }

  public final void onSizeChanged(int size)
  {
    mSize = size;
    updateDrawingParams();
  }
}
