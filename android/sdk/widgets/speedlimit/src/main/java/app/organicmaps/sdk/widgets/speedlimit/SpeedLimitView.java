package app.organicmaps.sdk.widgets.speedlimit;

import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.Color;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;
import androidx.annotation.AttrRes;
import androidx.annotation.ColorInt;
import androidx.annotation.IntRange;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StyleRes;
import java.util.EnumMap;
import java.util.Objects;

public class SpeedLimitView extends View
{
  public enum Shape
  {
    Circle,
    Square
  }

  public enum Mode
  {
    Normal,
    Warning,
    Alert
  }

  public record Colors(@ColorInt int backgroundColor, @ColorInt int borderColor, @ColorInt int textColor) {}

  private int mValue;
  private float mBorderWidthRatio;
  private float mCornerRadiusRatio;
  @NonNull
  private Shape mShape;
  @NonNull
  private Mode mMode;
  @NonNull
  private final EnumMap<Mode, Colors> mColors = new EnumMap<>(Mode.class);
  @NonNull
  private ShapeDrawer mDrawer;

  public SpeedLimitView(@NonNull Context context)
  {
    this(context, null);
  }

  public SpeedLimitView(@NonNull Context context, @Nullable AttributeSet attrs)
  {
    this(context, attrs, 0);
  }

  public SpeedLimitView(@NonNull Context context, @Nullable AttributeSet attrs, @AttrRes int defStyleAttr)
  {
    this(context, attrs, defStyleAttr, R.style.OMSpeedLimitView_Vienna);
  }

  public SpeedLimitView(@NonNull Context context, @Nullable AttributeSet attrs, @AttrRes int defStyleAttr,
                        @StyleRes int defStyleRes)
  {
    super(context, attrs, defStyleAttr, defStyleRes);

    obtainStyledAttributes(context, attrs, defStyleAttr, defStyleRes);
    initDrawer();
  }

  public void setStyle(@StyleRes int styleRes)
  {
    obtainStyledAttributes(getContext(), null, 0, styleRes);
    initDrawer();
  }

  public void setValue(@IntRange(from = 0, to = 999) int value)
  {
    mValue = Math.min(Math.max(value, 0), 999);
    mDrawer.setValue(Integer.toString(mValue));
    invalidate();
  }

  public void setMode(@NonNull Mode mode)
  {
    Objects.requireNonNull(mode);
    if (mMode != mode)
    {
      mMode = mode;
      updateCurrentColors();
      invalidate();
    }
  }

  @Override
  public boolean performClick()
  {
    super.performClick();
    return false;
  }

  @Override
  public boolean onTouchEvent(@NonNull MotionEvent event)
  {
    final boolean handled = mDrawer.onTouchEvent(event);
    if (handled)
    {
      performClick();
    }
    return handled;
  }

  @Override
  protected void onDraw(@NonNull Canvas canvas)
  {
    mDrawer.onDraw(canvas);
  }

  @Override
  protected void onSizeChanged(int w, int h, int oldw, int oldh)
  {
    super.onSizeChanged(w, h, oldw, oldh);

    final float paddingX = (float) (getPaddingLeft() + getPaddingRight());
    final float paddingY = (float) (getPaddingTop() + getPaddingBottom());

    mDrawer.setSize((float) w - paddingX, (float) h - paddingY);
  }

  private void obtainStyledAttributes(@NonNull Context context, @Nullable AttributeSet attrs, @AttrRes int defStyleAttr,
                                      @StyleRes int defStyleRes)
  {
    try (final TypedArray data =
             context.getTheme().obtainStyledAttributes(attrs, R.styleable.OMSpeedLimitView, defStyleAttr, defStyleRes))
    {
      mValue = data.getInt(R.styleable.OMSpeedLimitView_om_speed_limit_view_value, 60);
      mValue = Math.min(Math.max(mValue, 0), 999);
      mShape = Shape.values()[data.getInt(R.styleable.OMSpeedLimitView_om_speed_limit_view_shape, 0)];
      mMode = Mode.values()[data.getInt(R.styleable.OMSpeedLimitView_om_speed_limit_view_mode, 0)];

      mBorderWidthRatio =
          data.getFraction(R.styleable.OMSpeedLimitView_om_speed_limit_view_border_width_ratio, 1, 1, 0.1f);
      mCornerRadiusRatio =
          data.getFraction(R.styleable.OMSpeedLimitView_om_speed_limit_view_corner_radius_ratio, 1, 1, 0.15f);

      // Normal mode
      {
        final int backgroundColor =
            data.getColor(R.styleable.OMSpeedLimitView_om_speed_limit_view_background_color, Color.WHITE);
        final int borderColor = data.getColor(R.styleable.OMSpeedLimitView_om_speed_limit_view_border_color, Color.RED);
        final int textColor = data.getColor(R.styleable.OMSpeedLimitView_om_speed_limit_view_text_color, Color.BLACK);
        mColors.put(Mode.Normal, new Colors(backgroundColor, borderColor, textColor));
      }

      // Warning mode
      {
        final int backgroundColor =
            data.getColor(R.styleable.OMSpeedLimitView_om_speed_limit_view_warning_background_color, Color.WHITE);
        final int borderColor =
            data.getColor(R.styleable.OMSpeedLimitView_om_speed_limit_view_warning_border_color, Color.YELLOW);
        final int textColor =
            data.getColor(R.styleable.OMSpeedLimitView_om_speed_limit_view_warning_text_color, Color.YELLOW);
        mColors.put(Mode.Warning, new Colors(backgroundColor, borderColor, textColor));
      }

      // Alert mode
      {
        final int backgroundColor =
            data.getColor(R.styleable.OMSpeedLimitView_om_speed_limit_view_alert_background_color, Color.RED);
        final int borderColor =
            data.getColor(R.styleable.OMSpeedLimitView_om_speed_limit_view_alert_border_color, Color.RED);
        final int textColor =
            data.getColor(R.styleable.OMSpeedLimitView_om_speed_limit_view_alert_text_color, Color.WHITE);
        mColors.put(Mode.Alert, new Colors(backgroundColor, borderColor, textColor));
      }
    }
  }

  private void updateCurrentColors()
  {
    mDrawer.setColors(Objects.requireNonNull(mColors.get(mMode)));
  }

  private void initDrawer()
  {
    if (mShape == Shape.Circle)
      mDrawer = new CircleShapeDrawer(mBorderWidthRatio, mCornerRadiusRatio);
    else
      mDrawer = new SquareShapeDrawer(mBorderWidthRatio, mCornerRadiusRatio);
    mDrawer.setValue(Integer.toString(mValue));
    updateCurrentColors();
  }
}
