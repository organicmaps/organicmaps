package com.mapswithme.maps.widget;

import android.content.Context;
import android.graphics.Canvas;
import android.text.Layout;
import android.util.AttributeSet;
import android.widget.TextView;

public class LineCountTextView extends TextView
{
  public interface OnLineCountCalculatedListener
  {

    void onLineCountCalculated(boolean grater);
  }

  private OnLineCountCalculatedListener mListener;

  public LineCountTextView(Context context)
  {
    super(context);
  }

  public LineCountTextView(Context context, AttributeSet attrs)
  {
    super(context, attrs);
  }

  public LineCountTextView(Context context, AttributeSet attrs, int defStyleAttr)
  {
    super(context, attrs, defStyleAttr);
  }

  @Override
  protected void onDraw(Canvas canvas)
  {
    super.onDraw(canvas);
    Layout layout = getLayout();

    if (layout != null)
    {
      int textHeight = layout.getHeight();
      int viewHeight = getHeight();

      if (mListener != null)
      {
        mListener.onLineCountCalculated(textHeight > viewHeight);
      }
    }
  }

  public void setListener(OnLineCountCalculatedListener listener)
  {
    mListener = listener;
  }
}
