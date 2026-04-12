package app.organicmaps.widget;

import android.content.Context;
import android.util.AttributeSet;
import android.widget.LinearLayout;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

public class SuppressLayoutLinearLayout extends LinearLayout
{
  private boolean mSuppressLayout;

  public SuppressLayoutLinearLayout(@NonNull Context context)
  {
    super(context);
  }

  public SuppressLayoutLinearLayout(@NonNull Context context, @Nullable AttributeSet attrs)
  {
    super(context, attrs);
  }

  public SuppressLayoutLinearLayout(@NonNull Context context, @Nullable AttributeSet attrs, int defStyleAttr)
  {
    super(context, attrs, defStyleAttr);
  }

  public void setSuppressLayout(boolean suppress)
  {
    mSuppressLayout = suppress;
    if (!suppress && isLaidOut())
    {
      forceLayout();
      int widthSpec = MeasureSpec.makeMeasureSpec(getWidth(), MeasureSpec.EXACTLY);
      int heightSpec = MeasureSpec.makeMeasureSpec(getHeight(), MeasureSpec.EXACTLY);
      measure(widthSpec, heightSpec);
      layout(getLeft(), getTop(), getRight(), getBottom());
    }
  }

  @Override
  public void requestLayout()
  {
    if (!mSuppressLayout)
      super.requestLayout();
  }
}
