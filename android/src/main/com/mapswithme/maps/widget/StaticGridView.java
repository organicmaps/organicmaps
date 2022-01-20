package com.mapswithme.maps.widget;

import android.content.Context;
import android.util.AttributeSet;
import android.widget.GridView;

public class StaticGridView extends GridView
{
  public StaticGridView(Context context)
  {
    super(context);
  }

  public StaticGridView(Context context, AttributeSet attrs)
  {
    super(context, attrs);
  }

  public StaticGridView(Context context, AttributeSet attrs, int defStyleAttr)
  {
    super(context, attrs, defStyleAttr);
  }

  @Override
  public void onMeasure(int widthMeasureSpec, int heightMeasureSpec)
  {
    super.onMeasure(widthMeasureSpec, MeasureSpec.makeMeasureSpec(MEASURED_SIZE_MASK, MeasureSpec.AT_MOST));
    getLayoutParams().height = getMeasuredHeight();
  }
}
