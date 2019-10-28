package com.mapswithme.maps.widget;

import android.content.Context;
import android.content.res.TypedArray;
import android.os.Handler;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.viewpager.widget.PagerAdapter;
import androidx.viewpager.widget.ViewPager;
import android.util.AttributeSet;
import android.view.MotionEvent;

import com.mapswithme.maps.R;

public class ParallaxBackgroundViewPager extends ViewPager
{

  private static final int DEFAULT_AUTO_SCROLL_PERIOD = 3000;
  private static final int CAROUSEL_ITEMS_MIN_COUNT = 2;
  private final int mAutoScrollPeriod;
  private final boolean mAutoScroll;
  @NonNull
  private final Handler mAutoScrollHandler;
  @NonNull
  private final Runnable mAutoScrollMessage;
  private int mCurrentPagePosition;

  public ParallaxBackgroundViewPager(@NonNull Context context)
  {
    this(context, null);
  }

  public ParallaxBackgroundViewPager(@NonNull Context context, @Nullable AttributeSet attrs)
  {
    super(context, attrs);
    TypedArray array = context.getTheme()
                              .obtainStyledAttributes(attrs, R.styleable.ParallaxViewPagerBg, 0, 0);
    try
    {
      mAutoScroll = array.getBoolean(R.styleable.ParallaxViewPagerBg_autoScroll, false);
      mAutoScrollPeriod = array.getInt(R.styleable.ParallaxViewPagerBg_scrollPeriod,
                                       DEFAULT_AUTO_SCROLL_PERIOD);
    }
    finally
    {
      array.recycle();
    }
    mAutoScrollHandler = new Handler();
    mAutoScrollMessage = new AutoScrollMessage();
    OnPageChangeListener autoScrollPageListener = new AutoScrollPageListener();
    addOnPageChangeListener(autoScrollPageListener);
  }

  @Override
  protected void onDetachedFromWindow()
  {
    super.onDetachedFromWindow();
    stopAutoScroll();
    clearOnPageChangeListeners();
  }

  @Override
  public boolean dispatchTouchEvent(MotionEvent ev)
  {
    int action = ev.getAction();

    if (action == MotionEvent.ACTION_DOWN && mAutoScroll)
      stopAutoScroll();
    else if (action == MotionEvent.ACTION_UP || action == MotionEvent.ACTION_OUTSIDE)
      startAutoScroll();

    return super.dispatchTouchEvent(ev);
  }

  public void startAutoScroll()
  {
    mAutoScrollHandler.postDelayed(mAutoScrollMessage, mAutoScrollPeriod);
  }

  public void stopAutoScroll()
  {
    mAutoScrollHandler.removeCallbacks(mAutoScrollMessage);
  }

  private boolean isLastAutoScrollPosition()
  {
    PagerAdapter adapter = getAdapter();
    return adapter != null && adapter.getCount() - 1 == mCurrentPagePosition;
  }

  private class AutoScrollPageListener implements OnPageChangeListener
  {
    @Override
    public void onPageScrolled(int position, float positionOffset, int positionOffsetPixels)
    {
    }

    @Override
    public void onPageSelected(int position)
    {
      mCurrentPagePosition = position;
      mAutoScrollHandler.removeCallbacks(mAutoScrollMessage);
      mAutoScrollHandler.postDelayed(mAutoScrollMessage, mAutoScrollPeriod);
    }

    @Override
    public void onPageScrollStateChanged(int state)
    {
    }
  }

  private class AutoScrollMessage implements Runnable
  {
    @Override
    public void run()
    {
      PagerAdapter adapter = getAdapter();
      if (adapter == null
          || adapter.getCount() < CAROUSEL_ITEMS_MIN_COUNT
          || !mAutoScroll)
        return;

      if (isLastAutoScrollPosition())
        mCurrentPagePosition = 0;
      else
        mCurrentPagePosition++;

      setCurrentItem(mCurrentPagePosition, true);
    }
  }
}
