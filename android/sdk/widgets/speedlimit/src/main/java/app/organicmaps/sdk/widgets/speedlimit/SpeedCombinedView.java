package app.organicmaps.sdk.widgets.speedlimit;

import android.content.Context;
import android.content.res.TypedArray;
import android.util.AttributeSet;
import android.widget.FrameLayout;
import androidx.annotation.AttrRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StyleRes;
import java.util.Objects;

public class SpeedCombinedView extends FrameLayout
{
  @NonNull
  private final BackgroundView mBackgroundView;
  @NonNull
  private final CurrentSpeedView mCurrentSpeedView;
  @NonNull
  private final SpeedLimitView mSpeedLimitView;

  public SpeedCombinedView(@NonNull Context context)
  {
    this(context, null);
  }

  public SpeedCombinedView(@NonNull Context context, @Nullable AttributeSet attrs)
  {
    this(context, attrs, 0);
  }

  public SpeedCombinedView(@NonNull Context context, @Nullable AttributeSet attrs, @AttrRes int defStyleAttr)
  {
    this(context, attrs, defStyleAttr, R.style.OMSpeedCombinedView_Vienna);
  }

  public SpeedCombinedView(@NonNull Context context, @Nullable AttributeSet attrs, @AttrRes int defStyleAttr,
                           @StyleRes int defStyleRes)
  {
    super(context, attrs, defStyleAttr, defStyleRes);

    inflate(context, R.layout.__speed_combined_view, this);
    mBackgroundView = findViewById(R.id.__background_view);
    mCurrentSpeedView = findViewById(R.id.__current_speed_view);
    mSpeedLimitView = findViewById(R.id.__speed_limit_view);

    obtainStyledAttributes(context, attrs, defStyleAttr, defStyleRes);
  }

  @Override
  protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec)
  {
    final int availableWidth = MeasureSpec.getSize(widthMeasureSpec);
    final int availableHeight = MeasureSpec.getSize(heightMeasureSpec);

    final boolean isCurrentSpeedVisible = mCurrentSpeedView.getVisibility() != GONE;
    final boolean isSpeedLimitVisible = mSpeedLimitView.getVisibility() != GONE;
    final int visibleViewsCount = (isCurrentSpeedVisible ? 1 : 0) + (isSpeedLimitVisible ? 1 : 0);

    mBackgroundView.setVisibility(visibleViewsCount == 0 ? GONE : VISIBLE);

    if (visibleViewsCount == 0)
    {
      super.onMeasure(MeasureSpec.makeMeasureSpec(0, MeasureSpec.EXACTLY),
                      MeasureSpec.makeMeasureSpec(0, MeasureSpec.EXACTLY));
      return;
    }

    final int squareSize = visibleViewsCount == 2 ? Math.min(availableHeight, availableWidth / 2)
                                                  : Math.min(availableHeight, availableWidth);

    final int measuredWidth = visibleViewsCount * squareSize;

    super.onMeasure(MeasureSpec.makeMeasureSpec(measuredWidth, MeasureSpec.EXACTLY),
                    MeasureSpec.makeMeasureSpec(squareSize, MeasureSpec.EXACTLY));
  }

  private void obtainStyledAttributes(@NonNull Context context, @Nullable AttributeSet attrs, @AttrRes int defStyleAttr,
                                      @StyleRes int defStyleRes)
  {
    try (final TypedArray data = context.getTheme().obtainStyledAttributes(attrs, R.styleable.OMSpeedCombinedView,
                                                                           defStyleAttr, defStyleRes))
    {
      mSpeedLimitView.setValue(data.getInt(R.styleable.OMSpeedCombinedView_om_speed_combined_view_speed_limit, 90));
      mCurrentSpeedView.setValue(data.getInt(R.styleable.OMSpeedCombinedView_om_speed_combined_view_current_speed, 60));
      mCurrentSpeedView.setUnits(Objects.requireNonNullElse(
          data.getString(R.styleable.OMSpeedCombinedView_om_speed_combined_view_current_speed_units), "km/h"));
      mBackgroundView.setShape(
          Shape.values()[data.getInt(R.styleable.OMSpeedCombinedView_om_speed_combined_view_shape, 0)]);
      mSpeedLimitView.setStyle(
          data.getResourceId(R.styleable.OMSpeedCombinedView_om_speed_combined_view_speed_limit_style,
                             R.style.OMSpeedCombinedView_Vienna));
      //      mMode =
      //      SpeedLimitView.Mode.values()[data.getInt(R.styleable.OMSpeedLimitView_om_speed_limit_view_mode, 0)];
      //
      //      mBorderWidthRatio =
      //          data.getFraction(R.styleable.OMSpeedLimitView_om_speed_limit_view_border_width_ratio, 1, 1,
      //          0.1f);
      //      mCornerRadiusRatio =
      //          data.getFraction(R.styleable.OMSpeedLimitView_om_speed_limit_view_corner_radius_ratio, 1, 1,
      //          0.15f);
      //
      //      // Normal mode
      //      {
      //        final int backgroundColor =
      //            data.getColor(R.styleable.OMSpeedLimitView_om_speed_limit_view_background_color, Color.WHITE);
      //        final int borderColor =
      //        data.getColor(R.styleable.OMSpeedLimitView_om_speed_limit_view_border_color, Color.RED); final int
      //        textColor = data.getColor(R.styleable.OMSpeedLimitView_om_speed_limit_view_text_color,
      //        Color.BLACK); mColors.put(SpeedLimitView.Mode.Normal, new SpeedLimitView.Colors(backgroundColor,
      //        borderColor, textColor));
      //      }
      //
      //      // Warning mode
      //      {
      //        final int backgroundColor =
      //            data.getColor(R.styleable.OMSpeedLimitView_om_speed_limit_view_warning_background_color,
      //            Color.WHITE);
      //        final int borderColor =
      //            data.getColor(R.styleable.OMSpeedLimitView_om_speed_limit_view_warning_border_color,
      //            Color.YELLOW);
      //        final int textColor =
      //            data.getColor(R.styleable.OMSpeedLimitView_om_speed_limit_view_warning_text_color,
      //            Color.YELLOW);
      //        mColors.put(SpeedLimitView.Mode.Warning, new SpeedLimitView.Colors(backgroundColor, borderColor,
      //        textColor));
      //      }
      //
      //      // Alert mode
      //      {
      //        final int backgroundColor =
      //            data.getColor(R.styleable.OMSpeedLimitView_om_speed_limit_view_alert_background_color,
      //            Color.RED);
      //        final int borderColor =
      //            data.getColor(R.styleable.OMSpeedLimitView_om_speed_limit_view_alert_border_color, Color.RED);
      //        final int textColor =
      //            data.getColor(R.styleable.OMSpeedLimitView_om_speed_limit_view_alert_text_color, Color.WHITE);
      //        mColors.put(SpeedLimitView.Mode.Alert, new SpeedLimitView.Colors(backgroundColor, borderColor,
      //        textColor));
      //      }
    }
  }
}
