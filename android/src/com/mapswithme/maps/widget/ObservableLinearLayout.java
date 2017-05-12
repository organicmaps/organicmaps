package com.mapswithme.maps.widget;

import android.annotation.TargetApi;
import android.content.Context;
import android.os.Build;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.util.AttributeSet;
import android.widget.LinearLayout;

public class ObservableLinearLayout extends LinearLayout
{
  @Nullable
  private SizeChangedListener mSizeChangedListener;

  public ObservableLinearLayout(@NonNull Context context)
  {
    super(context);
  }

  public ObservableLinearLayout(@NonNull Context context, @Nullable AttributeSet attrs)
  {
    super(context, attrs);
  }

  public ObservableLinearLayout(@NonNull Context context, @Nullable AttributeSet attrs, int defStyleAttr)
  {
    super(context, attrs, defStyleAttr);
  }

  @TargetApi(Build.VERSION_CODES.LOLLIPOP)
  public ObservableLinearLayout(@NonNull Context context, @Nullable AttributeSet attrs, int defStyleAttr, int defStyleRes)
  {
    super(context, attrs, defStyleAttr, defStyleRes);
  }

  @Override
  protected void onSizeChanged(int w, int h, int oldw, int oldh)
  {
    super.onSizeChanged(w, h, oldw, oldh);
    if (mSizeChangedListener != null)
      mSizeChangedListener.onSizeChanged(w, h, oldw, oldh);
  }

  public void setSizeChangedListener(@Nullable SizeChangedListener listener)
  {
    mSizeChangedListener = listener;
  }

  public interface SizeChangedListener
  {
    void onSizeChanged(int width, int height, int oldWidth, int oldHeight);
  }
}
