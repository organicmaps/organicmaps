package com.mapswithme.maps.widget;

import android.content.Context;
import android.util.AttributeSet;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.util.UiUtils;

import java.util.ArrayList;
import java.util.List;

/**
 * FrameLayout which presses out the children which can not be fully fit by height.<br/>
 * Child views should be marked with {@code @string/tag_height_limited} tag.
 */
public class HeightLimitedFrameLayout extends FrameLayout
{
  private static final String TAG_LIMIT = MwmApplication.get().getString(R.string.tag_height_limited);
  private final List<View> mLimitedViews = new ArrayList<>();

  public HeightLimitedFrameLayout(Context context, AttributeSet attrs)
  {
    super(context, attrs);
  }

  private void collectViews(View v)
  {
    if (TAG_LIMIT.equals(v.getTag()))
    {
      mLimitedViews.add(v);
      return;
    }

    if (v instanceof ViewGroup)
    {
      final ViewGroup vg = (ViewGroup) v;
      for (int i = 0; i < vg.getChildCount(); i++)
        collectViews(vg.getChildAt(i));
    }
  }

  @Override
  protected void onFinishInflate()
  {
    super.onFinishInflate();

    if (!isInEditMode())
      collectViews(this);
  }

  @Override
  protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec)
  {
    if (isInEditMode())
    {
      super.onMeasure(widthMeasureSpec, heightMeasureSpec);
      return;
    }

    for (View v : mLimitedViews)
      UiUtils.show(v);

    super.onMeasure(widthMeasureSpec, MeasureSpec.makeMeasureSpec(0, MeasureSpec.UNSPECIFIED));

    if (getMeasuredHeight() > MeasureSpec.getSize(heightMeasureSpec))
      for (View v : mLimitedViews)
        UiUtils.hide(v);

    super.onMeasure(widthMeasureSpec, heightMeasureSpec);
  }
}
