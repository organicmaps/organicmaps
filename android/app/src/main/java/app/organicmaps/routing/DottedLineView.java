package app.organicmaps.routing;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.util.AttributeSet;
import android.util.TypedValue;
import android.view.View;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.R;
import app.organicmaps.util.ThemeUtils;

/**
 * A vertical column of evenly spaced dots, used as the walking-segment connector on the transit details
 * timeline (the dashed counterpart to the ride legs' solid colored bar). Drawn manually because an XML
 * shape/line can't render a dashed line of arbitrary height in a thin column.
 */
public class DottedLineView extends View
{
  // Two dots above the central walk-icon gap and two below it.
  private static final int DOTS_PER_SIDE = 2;

  @NonNull
  private final Paint mPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
  private final float mRadius;
  private final float mStep;
  // Distance from the centre to the first dot: clears the walk icon that sits in the middle.
  private final float mGap;

  public DottedLineView(@NonNull Context context, @Nullable AttributeSet attrs)
  {
    super(context, attrs);
    mPaint.setColor(ThemeUtils.getColor(context, R.attr.secondary));
    mRadius = dp(2.5f);
    mStep = mRadius * 2 + dp(4f);
    mGap = dp(18f);
  }

  private float dp(float value)
  {
    return TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, value, getResources().getDisplayMetrics());
  }

  @Override
  protected void onDraw(@NonNull Canvas canvas)
  {
    final float cx = getWidth() / 2f;
    final float center = getHeight() / 2f;
    for (int i = 1; i <= DOTS_PER_SIDE; i++)
    {
      final float offset = mGap + (i - 1) * mStep;
      canvas.drawCircle(cx, center - offset, mRadius, mPaint);
      canvas.drawCircle(cx, center + offset, mRadius, mPaint);
    }
  }
}
