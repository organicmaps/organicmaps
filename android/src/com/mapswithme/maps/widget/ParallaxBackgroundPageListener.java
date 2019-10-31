package com.mapswithme.maps.widget;

import android.app.Activity;
import android.view.View;
import android.widget.ImageView;

import androidx.annotation.NonNull;
import androidx.fragment.app.FragmentActivity;
import androidx.viewpager.widget.ViewPager;

import java.util.Collections;
import java.util.List;

public class ParallaxBackgroundPageListener implements ViewPager.OnPageChangeListener
{

  private static final float MIDDLE_POSITION_OFFSET = 0.5f;
  private static final int MIDDLE_POSITION_OFFSET_PIXELS = 1;
  private static final float ALPHA_TRANSPARENT = 0;
  private static final float ALPHA_OPAQUE = 1;
  private static final int MINUS_INFINITY_EDGE = -1;
  private static final int PLUS_INFINITY_EDGE = 1;
  private static final float SETTLED_PAGE_POSITION = 0.0f;

  @NonNull
  private final ViewPager mPager;
  @NonNull
  private final Activity mActivity;
  @NonNull
  private final List<Integer> mItems;

  private int mCurrentPagePosition;
  private boolean mScrollToRight = true;
  private boolean mScrollStarted;
  private boolean mShouldCalculateScrollDirection;
  @NonNull
  private final PageViewProvider mPageViewProvider;

  ParallaxBackgroundPageListener(@NonNull Activity activity,
                                 @NonNull ViewPager pager,
                                 @NonNull List<Integer> items,
                                 @NonNull PageViewProvider pageViewProvider)
  {
    mPager = pager;
    mActivity = activity;
    mItems = Collections.unmodifiableList(items);
    mPageViewProvider = pageViewProvider;
  }

  public ParallaxBackgroundPageListener(@NonNull FragmentActivity activity,
                                        @NonNull ViewPager pager,
                                        @NonNull List<Integer> items)
  {
    this(activity, pager, items, PageViewProviderFactory.defaultProvider(activity, pager));
  }

  @Override
  public void onPageScrollStateChanged(int state)
  {
    boolean isIdle = state == ViewPager.SCROLL_STATE_IDLE;
    if (isIdle)
      setIdlePosition();

    mScrollStarted  = isIdle && !mScrollStarted;

    if (mScrollStarted)
      mShouldCalculateScrollDirection = true;
  }

  private void setIdlePosition()
  {
    mCurrentPagePosition = mPager.getCurrentItem();
  }

  @Override
  public void onPageSelected(int position)
  {
    if (position == 0)
      onPageScrollStateChanged(ViewPager.SCROLL_STATE_IDLE);

    if (Math.abs(mCurrentPagePosition - position) > 1)
      mCurrentPagePosition = mScrollToRight ? Math.max(0, position - 1)
                                            : Math.min(position + 1, mItems.size() - 1);
  }

  @Override
  public void onPageScrolled(int position, float positionOffset, int positionOffsetPixels)
  {
    if (mShouldCalculateScrollDirection)
    {
      mScrollToRight =
          MIDDLE_POSITION_OFFSET > positionOffset && positionOffsetPixels > MIDDLE_POSITION_OFFSET_PIXELS;
      mShouldCalculateScrollDirection = false;
    }

    int scrollX = mPager.getScrollX();
    int animatedItemIndex = mScrollToRight ? Math.min(mCurrentPagePosition, mItems.size() - 1)
                                           : Math.max(0, mCurrentPagePosition - 1);
    setAlpha(animatedItemIndex, scrollX);

    if (scrollX == 0)
      restoreInitialAlphaValues();
  }

  private void setAlpha(int animatedItemIndex, int scrollX)
  {
    View view = mPageViewProvider.findViewByIndex(animatedItemIndex);
    if (view == null)
      return;

    ViewPager.LayoutParams lp = (ViewPager.LayoutParams) view.getLayoutParams();
    if (lp.isDecor)
      return;

    float transformPos = (float) (view.getLeft() - scrollX) / (float) getPagerWidth();
    ImageView currentImage = mActivity.findViewById(mItems.get(animatedItemIndex));
    if (transformPos <= MINUS_INFINITY_EDGE || transformPos >= PLUS_INFINITY_EDGE)
      currentImage.setAlpha(ALPHA_TRANSPARENT);
    else if (transformPos == SETTLED_PAGE_POSITION)
      currentImage.setAlpha(ALPHA_OPAQUE);
    else
      currentImage.setAlpha(ALPHA_OPAQUE - Math.abs(transformPos));
  }

  private void restoreInitialAlphaValues()
  {
    for (int j = mItems.size() - 1; j >= 0; j--)
    {
      View view = mActivity.findViewById(mItems.get(j));
      view.setAlpha(ALPHA_OPAQUE);
    }
  }

  private int getPagerWidth()
  {
    return mPager.getMeasuredWidth() - mPager.getPaddingLeft() - mPager.getPaddingRight();
  }
}
