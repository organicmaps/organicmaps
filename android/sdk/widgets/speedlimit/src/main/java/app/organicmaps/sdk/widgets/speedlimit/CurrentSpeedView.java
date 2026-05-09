package app.organicmaps.sdk.widgets.speedlimit;

import android.content.Context;
import android.util.AttributeSet;
import android.widget.FrameLayout;
import android.widget.TextView;
import androidx.annotation.AttrRes;
import androidx.annotation.IntRange;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StyleRes;
import androidx.core.widget.TextViewCompat;

public class CurrentSpeedView extends FrameLayout
{
  @NonNull
  private final TextView mCurrentSpeed;
  @NonNull
  private final TextView mUnits;

  public CurrentSpeedView(@NonNull Context context)
  {
    this(context, null);
  }

  public CurrentSpeedView(@NonNull Context context, @Nullable AttributeSet attrs)
  {
    this(context, attrs, 0);
  }

  public CurrentSpeedView(@NonNull Context context, @Nullable AttributeSet attrs, @AttrRes int defStyleAttr)
  {
    this(context, attrs, defStyleAttr, 0);
  }

  public CurrentSpeedView(@NonNull Context context, @Nullable AttributeSet attrs, @AttrRes int defStyleAttr,
                          @StyleRes int defStyleRes)
  {
    super(context, attrs, defStyleAttr, defStyleRes);

    inflate(context, R.layout.__current_speed_view, this);
    mCurrentSpeed = findViewById(R.id.__current_speed_view_current_speed);
    mUnits = findViewById(R.id.__current_speed_view_units);

    TextViewCompat.setAutoSizeTextTypeUniformWithConfiguration(mCurrentSpeed, 1, 1000, 1,
                                                               TextViewCompat.AUTO_SIZE_TEXT_TYPE_UNIFORM);
    TextViewCompat.setAutoSizeTextTypeUniformWithConfiguration(mUnits, 1, 1000, 1,
                                                               TextViewCompat.AUTO_SIZE_TEXT_TYPE_UNIFORM);
  }

  public void setValue(@IntRange(from = 0, to = 999) int value)
  {
    mCurrentSpeed.setText(String.valueOf(value));
  }

  public void setUnits(@NonNull String units)
  {
    mUnits.setText(units);
  }
}
