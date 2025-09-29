package app.organicmaps.sdk.widgets.roadsign.drawers;

import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import androidx.annotation.CallSuper;
import androidx.annotation.ColorInt;
import androidx.annotation.NonNull;
import androidx.annotation.RestrictTo;
import androidx.annotation.RestrictTo.Scope;

@RestrictTo(Scope.LIBRARY)
public abstract sealed class ContentDrawer permits TextDrawer
{
  public static class Config
  {
    @ColorInt
    public int color = Color.BLACK;
  }

  @NonNull
  protected final Paint mPaint = new Paint(Paint.ANTI_ALIAS_FLAG);

  @NonNull
  protected final Config mConfig = new Config();

  protected float mRadiusBound = 0f;

  public void setColor(@ColorInt int color)
  {
    mConfig.color = color;
  }

  @CallSuper
  public void onSizeChanged(float radiusBound)
  {
    mRadiusBound = radiusBound;
  }

  public abstract void draw(@NonNull Canvas canvas, float cx, float cy);
}
