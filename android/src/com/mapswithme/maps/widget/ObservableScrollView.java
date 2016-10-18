package com.mapswithme.maps.widget;

import android.content.Context;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.view.GestureDetectorCompat;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.widget.ScrollView;

public class ObservableScrollView extends ScrollView
{
  public interface ScrollListener
  {
    void onScroll(int left, int top);
    void onScrollEnd();
  }

  public static class SimpleScrollListener implements ScrollListener
  {
    @Override
    public void onScroll(int left, int top) {}

    @Override
    public void onScrollEnd() {}
  }

  private static final int SCROLL_END_DETECT_INTERVAL = 100;

  private final Runnable mScrollEndDetector = new Runnable()
  {
    @Override
    public void run()
    {
      if (mTouched)
        return;

      if (mPrevScroll == getScrollY())
      {
        if (mScrollListener != null)
          mScrollListener.onScrollEnd();
        return;
      }

      mPrevScroll = getScrollY();
      postDelayed(this, SCROLL_END_DETECT_INTERVAL);
    }
  };

  private ScrollListener mScrollListener;
  private int mPrevScroll;
  private boolean mTouched;
  @Nullable
  private GestureDetectorCompat mGestureDetector;


  public ObservableScrollView(Context context)
  {
    super(context);
  }

  public ObservableScrollView(Context context, AttributeSet attrs)
  {
    super(context, attrs);
  }

  public ObservableScrollView(Context context, AttributeSet attrs, int defStyle)
  {
    super(context, attrs, defStyle);
  }

  /**
   * Translate all {@link MotionEvent}s to specified  {@link GestureDetectorCompat}
   * all consuming flags from GestureDetectorCompat.onTouchEvent are ignored.
   *
   * @param gestureDetector {@link GestureDetectorCompat} to use.
   */
  public void setGestureDetector(@Nullable GestureDetectorCompat gestureDetector)
  {
    mGestureDetector = gestureDetector;
  }

  private boolean shouldSkipEvent(MotionEvent ev)
  {
    return (mTouched &&
            (ev.getActionMasked() == MotionEvent.ACTION_MOVE) &&
            getScrollY() == mPrevScroll);
  }

  @Override
  protected void onScrollChanged(int curLeft, int curTop, int prevLeft, int prevTop)
  {
    super.onScrollChanged(curLeft, curTop, prevLeft, prevTop);
    if (mScrollListener != null)
      mScrollListener.onScroll(curLeft, curTop);
  }

  @Override
  public boolean onTouchEvent(@NonNull MotionEvent ev)
  {
    if (mGestureDetector != null)
      mGestureDetector.onTouchEvent(ev);

    if (!super.onTouchEvent(ev))
      return false;

    if (shouldSkipEvent(ev))
      return true;

    switch (ev.getAction() & MotionEvent.ACTION_MASK)
    {
    case MotionEvent.ACTION_DOWN:
      mTouched = true;
      break;

    case MotionEvent.ACTION_UP:
    case MotionEvent.ACTION_CANCEL:
      mTouched = false;

      if (mScrollListener != null)
      {
        mPrevScroll = getScrollY();
        postDelayed(mScrollEndDetector, SCROLL_END_DETECT_INTERVAL);
      }
      break;
    }

    return true;
  }

  public void setScrollListener(ScrollListener scrollListener)
  {
    mScrollListener = scrollListener;
  }
}