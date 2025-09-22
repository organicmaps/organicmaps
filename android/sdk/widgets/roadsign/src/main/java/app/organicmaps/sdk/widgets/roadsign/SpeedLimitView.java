package app.organicmaps.sdk.widgets.roadsign;

import android.content.Context;
import android.content.res.TypedArray;
import android.util.AttributeSet;
import androidx.annotation.AttrRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StyleRes;
import app.organicmaps.sdk.widgets.roadsign.drawers.TextDrawer;

public class SpeedLimitView extends RoadSignView
{
  @NonNull
  private final TextDrawer mTextDrawer = new TextDrawer();

  private int mSpeedLimit = 0;

  public SpeedLimitView(@NonNull Context context)
  {
    this(context, null);
  }

  public SpeedLimitView(@NonNull Context context, @Nullable AttributeSet attrs)
  {
    this(context, attrs, R.attr.speedLimitViewStyle);
  }

  public SpeedLimitView(@NonNull Context context, @Nullable AttributeSet attrs, @AttrRes int defStyleAttr)
  {
    this(context, attrs, defStyleAttr, R.style.SpeedLimitView_Default);
  }

  public SpeedLimitView(@NonNull Context context, @Nullable AttributeSet attrs, @AttrRes int defStyleAttr,
                        @StyleRes int defStyleRes)
  {
    super(context, attrs, defStyleAttr, defStyleRes);
    readStyleAttributes(attrs, defStyleAttr, defStyleRes);
    init();
  }

  public void setSpeedLimit(final int speedLimit, boolean alert)
  {
    final boolean speedLimitChanged = mSpeedLimit != speedLimit;

    mSpeedLimit = speedLimit;
    mAlert = alert;

    if (speedLimitChanged)
      mTextDrawer.setText(Integer.toString(mSpeedLimit));

    invalidate();
  }

  public int getSpeedLimit()
  {
    return mSpeedLimit;
  }

  public boolean isAlert()
  {
    return mAlert;
  }

  @Override
  protected void init()
  {
    super.init();
    if (isInEditMode())
      mTextDrawer.setText(Integer.toString(mSpeedLimit));
    setContentDrawer(mTextDrawer);
  }

  private void readStyleAttributes(@Nullable AttributeSet attrs, @AttrRes int defStyleAttr, @StyleRes int defStyleRes)
  {
    try (TypedArray data = getContext().getTheme().obtainStyledAttributes(attrs, R.styleable.SpeedLimitView,
                                                                          defStyleAttr, defStyleRes))
    {
      mConfig.shape = data.getInt(R.styleable.SpeedLimitView_shape, mConfig.shape.ordinal()) == 0 ? Config.Shape.Circle
                                                                                                  : Config.Shape.Square;

      mConfig.backgroundColor = data.getColor(R.styleable.SpeedLimitView_backgroundColor, mConfig.backgroundColor);
      mConfig.outlineColor = data.getColor(R.styleable.SpeedLimitView_outlineColor, mConfig.outlineColor);
      mConfig.edgeColor = data.getColor(R.styleable.SpeedLimitView_edgeColor, mConfig.edgeColor);

      mConfig.backgroundAlertColor =
          data.getColor(R.styleable.SpeedLimitView_backgroundAlertColor, mConfig.backgroundAlertColor);
      mConfig.outlineAlertColor =
          data.getColor(R.styleable.SpeedLimitView_outlineAlertColor, mConfig.outlineAlertColor);
      mConfig.edgeAlertColor = data.getColor(R.styleable.SpeedLimitView_edgeAlertColor, mConfig.edgeAlertColor);

      mConfig.edgeWidthRatio =
          data.getFraction(R.styleable.SpeedLimitView_edgeWidthRatio, 1, 1, mConfig.edgeWidthRatio);
      mConfig.outlineWidthRatio =
          data.getFraction(R.styleable.SpeedLimitView_outlineWidthRatio, 1, 1, mConfig.outlineWidthRatio);
      mConfig.cornerRadiusRatio =
          data.getFraction(R.styleable.SpeedLimitView_cornerRadiusRatio, 1, 1, mConfig.cornerRadiusRatio);

      mConfig.contentColor = data.getColor(R.styleable.SpeedLimitView_contentColor, mConfig.contentColor);
      mConfig.contentAlertColor =
          data.getColor(R.styleable.SpeedLimitView_contentAlertColor, mConfig.contentAlertColor);

      if (isInEditMode())
      {
        mAlert = data.getBoolean(R.styleable.SpeedLimitView_editModeAlert, mAlert);
        mSpeedLimit = data.getInt(R.styleable.SpeedLimitView_editModeSpeedLimit, 60);
      }
    }
  }
}
