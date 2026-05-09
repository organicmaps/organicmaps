package app.organicmaps.sdk.widgets.speedlimit;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.util.AttributeSet;
import android.view.View;
import android.view.ViewGroup;
import androidx.annotation.AttrRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RestrictTo;
import androidx.annotation.StyleRes;
import androidx.constraintlayout.widget.ConstraintLayout;

@RestrictTo(RestrictTo.Scope.LIBRARY)
public class BackgroundView extends View
{
  @NonNull
  private final Paint mFillPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
  @NonNull
  private Shape mShape = Shape.Circle;

  public BackgroundView(@NonNull Context context)
  {
    this(context, null);
  }

  public BackgroundView(@NonNull Context context, @Nullable AttributeSet attrs)
  {
    this(context, attrs, 0);
  }

  public BackgroundView(@NonNull Context context, @Nullable AttributeSet attrs, @AttrRes int defStyleAttr)
  {
    this(context, attrs, defStyleAttr, 0);
  }

  public BackgroundView(@NonNull Context context, @Nullable AttributeSet attrs, @AttrRes int defStyleAttr,
                        @StyleRes int defStyleRes)
  {
    super(context, attrs, defStyleAttr, defStyleRes);
    mFillPaint.setColor(Color.WHITE);
    mFillPaint.setStyle(Paint.Style.FILL);
  }

  public void setShape(@NonNull Shape shape)
  {
    mShape = shape;
    invalidate();
  }

  @Override
  protected void onDraw(@NonNull Canvas canvas)
  {
    float radius;
    if (mShape == Shape.Circle)
      radius = Math.min(getWidth(), getHeight()) * 0.5f;
    else
      radius = Math.min(getWidth(), getHeight()) * 0.15f;
    canvas.drawRoundRect(0.0f, 0.0f, getWidth(), getHeight(), radius, radius, mFillPaint);
  }

  @Override
  protected void onAttachedToWindow()
  {
    super.onAttachedToWindow();

    final ConstraintLayout.LayoutParams lp = (ConstraintLayout.LayoutParams) getLayoutParams();
    final ViewGroup parent = (ViewGroup) getParent();
    final int padL = parent.getPaddingLeft();
    final int padT = parent.getPaddingTop();
    final int padR = parent.getPaddingRight();
    final int padB = parent.getPaddingBottom();
    lp.setMargins(-padL, -padT, -padR, -padB);
    setLayoutParams(lp);
  }
}
