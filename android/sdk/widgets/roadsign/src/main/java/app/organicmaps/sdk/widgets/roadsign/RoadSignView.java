package app.organicmaps.sdk.widgets.roadsign;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;
import androidx.annotation.AttrRes;
import androidx.annotation.CallSuper;
import androidx.annotation.ColorInt;
import androidx.annotation.FloatRange;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StyleRes;
import app.organicmaps.sdk.widgets.roadsign.drawers.BackgroundDrawer;
import app.organicmaps.sdk.widgets.roadsign.drawers.CircleBackgroundDrawer;
import app.organicmaps.sdk.widgets.roadsign.drawers.ContentDrawer;
import app.organicmaps.sdk.widgets.roadsign.drawers.SquareBackgroundDrawer;

public abstract class RoadSignView extends View
{
  public static class Config
  {
    public enum Shape
    {
      Circle,
      Square,
    }

    @NonNull
    public Shape shape = Shape.Circle;

    @ColorInt
    public int backgroundColor = Color.WHITE;
    @ColorInt
    public int outlineColor = Color.RED;
    @ColorInt
    public int edgeColor = Color.WHITE;
    @ColorInt
    public int contentColor = Color.BLACK;

    @ColorInt
    public int backgroundAlertColor = Color.RED;
    @ColorInt
    public int outlineAlertColor = Color.RED;
    @ColorInt
    public int edgeAlertColor = Color.WHITE;
    @ColorInt
    public int contentAlertColor = Color.BLACK;

    @FloatRange(from = 0.0, to = 0.5)
    public float edgeWidthRatio = 0.02f;
    @FloatRange(from = 0.0, to = 0.5)
    public float outlineWidthRatio = 0.1f;
    @FloatRange(from = 0.0, to = 0.5)
    public float cornerRadiusRatio = 0.1f;
  }

  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private BackgroundDrawer mBackgroundDrawer;
  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private ContentDrawer mContentDrawer;

  @NonNull
  protected final Config mConfig = new Config();

  protected boolean mAlert = false;

  public RoadSignView(@NonNull Context context, @Nullable AttributeSet attrs, @AttrRes int defStyleAttr,
                      @StyleRes int defStyleRes)
  {
    super(context, attrs, defStyleAttr, defStyleRes);
  }

  @Override
  public boolean onTouchEvent(@NonNull MotionEvent event)
  {
    if (mBackgroundDrawer.onTouch(event.getX(), event.getY()))
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

  protected void setContentDrawer(@NonNull ContentDrawer contentDrawer)
  {
    mContentDrawer = contentDrawer;
  }

  @CallSuper
  protected void init()
  {
    if (mConfig.shape == Config.Shape.Circle)
      mBackgroundDrawer = new CircleBackgroundDrawer();
    else if (mConfig.shape == Config.Shape.Square)
      mBackgroundDrawer = new SquareBackgroundDrawer();
    else
      throw new IllegalStateException("Unsupported shape: " + mConfig.shape);

    final BackgroundDrawer.Config drawerConfig = new BackgroundDrawer.Config();
    drawerConfig.backgroundColor = mConfig.backgroundColor;
    drawerConfig.outlineColor = mConfig.outlineColor;
    drawerConfig.edgeColor = mConfig.edgeColor;
    drawerConfig.edgeWidthRatio = mConfig.edgeWidthRatio;
    drawerConfig.outlineWidthRatio = mConfig.outlineWidthRatio;
    drawerConfig.cornerRadiusRatio = mConfig.cornerRadiusRatio;
    mBackgroundDrawer.setConfig(drawerConfig);
  }

  @Override
  protected void onDraw(@NonNull Canvas canvas)
  {
    if (mAlert)
    {
      mBackgroundDrawer.setColors(mConfig.backgroundAlertColor, mConfig.outlineAlertColor, mConfig.edgeAlertColor);
      mContentDrawer.setColor(mConfig.contentAlertColor);
    }
    else
    {
      mBackgroundDrawer.setColors(mConfig.backgroundColor, mConfig.outlineColor, mConfig.edgeColor);
      mContentDrawer.setColor(mConfig.contentColor);
    }

    final float cx = getWidth() / 2.0f;
    final float cy = getHeight() / 2.0f;

    mBackgroundDrawer.draw(canvas, cx, cy);
    mContentDrawer.draw(canvas, cx, cy);
  }

  @Override
  protected void onSizeChanged(int w, int h, int oldw, int oldh)
  {
    super.onSizeChanged(w, h, oldw, oldh);

    final int paddingX = getPaddingLeft() + getPaddingRight();
    final int paddingY = getPaddingTop() + getPaddingBottom();

    mBackgroundDrawer.onSizeChanged(Math.min(w - paddingX, h - paddingY));
    mContentDrawer.onSizeChanged(mBackgroundDrawer.getInnerRadius());
  }

  @Override
  protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec)
  {
    final int width = MeasureSpec.getSize(widthMeasureSpec);
    final int height = MeasureSpec.getSize(heightMeasureSpec);
    final int size = Math.min(width, height);
    setMeasuredDimension(size, size);
  }
}
