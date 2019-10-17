package com.mapswithme.maps.widget;

import android.support.annotation.NonNull;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentPagerAdapter;
import android.support.v4.view.ViewPager;
import android.support.v7.app.AppCompatActivity;
import android.view.View;
import android.widget.ImageView;

import java.util.List;
import java.util.NoSuchElementException;
import java.util.Objects;

public class ParallaxBackgroundPageListener implements ViewPager.OnPageChangeListener
{

  private static final String ANDROID_SWITCHER_TAG_SEGMENT = "android:switcher:";
  private static final String SEPARATOR_TAG_SEGMENT = ":";
  private static final float POSITION_OFFSET_BASE = 0.5f;
  private static final int POSITION_OFFSET_PIXELS_BASE = 1;
  private static final float ALPHA_TRANSPARENT = 0.0F;
  private static final float ALPHA_OPAQUE = 1.0f;
  private static final int MINUS_INFINITY_EDGE = -1;
  private static final int PLUS_INFINITY_EDGE = 1;
  private static final float SETTLED_PAGE_POSITION = 0.0f;

  @NonNull
  private final ViewPager mPager;
  @NonNull
  private final FragmentPagerAdapter mAdapter;
  @NonNull
  private final AppCompatActivity mActivity;
  @NonNull
  private final List<Integer> mItems;

  private int mCurrentPagePosition;
  private boolean mIsScrollToRight = true;
  private boolean mIsScrollStarted;
  private boolean mShouldCalculateScrollDirection;

  public ParallaxBackgroundPageListener(@NonNull AppCompatActivity activity,
                                        @NonNull ViewPager pager,
                                        @NonNull FragmentPagerAdapter adapter,
                                        @NonNull List<Integer> items)
  {
    mPager = pager;
    mAdapter = adapter;
    mActivity = activity;
    mItems = items;
  }

  @Override
  public void onPageScrollStateChanged(int state)
  {
    if (state == ViewPager.SCROLL_STATE_IDLE)
    {
      mCurrentPagePosition = mPager.getCurrentItem();
      mIsScrollToRight = true;
    }

    boolean isDragScroll = isDragScroll(state);

    mIsScrollStarted = isDragScroll;
    if (isDragScroll)
      mShouldCalculateScrollDirection = true;
  }

  @Override
  public void onPageSelected(int position)
  {
    if (position == 0)
      onPageScrollStateChanged(ViewPager.SCROLL_STATE_IDLE);
  }

  @Override
  public void onPageScrolled(int position, float positionOffset, int positionOffsetPixels)
  {
    recalculateScrollDirection(positionOffset, positionOffsetPixels);

    int scrollX = mPager.getScrollX();
    if (canScrollToLeft(scrollX) || isRightEdge(scrollX))
      return;

    int animatedItemIndex = mIsScrollToRight ? mCurrentPagePosition : mCurrentPagePosition - 1;
    setAlpha(animatedItemIndex, scrollX);

    if (isLeftEdge(scrollX))
      restoreInitialAlphaValues();
  }

  private boolean isDragScroll(int state)
  {
    return !mIsScrollStarted && state == ViewPager.SCROLL_STATE_DRAGGING;
  }

  private boolean canScrollToLeft(int scrollX)
  {
    return isLeftEdge(scrollX) && !mIsScrollToRight;
  }

  private void setAlpha(int animatedItemIndex, int scrollX)
  {
    View child = findFragmentViewByIndex(animatedItemIndex);
    ViewPager.LayoutParams lp = (ViewPager.LayoutParams) child.getLayoutParams();
    if (lp.isDecor)
      return;

    float transformPos = (float) (child.getLeft() - scrollX) / (float) getClientWidth();
    initCurrentAlpha(transformPos, animatedItemIndex);
  }

  @NonNull
  private View findFragmentViewByIndex(int index)
  {
    String tag = makePagerFragmentTag(index, mPager.getId());
    Fragment page = mActivity.getSupportFragmentManager().findFragmentByTag(tag);
    if (page == null)
      throw new NoSuchElementException("no such element for tag  : " + tag);

    return Objects.requireNonNull(page.getView());
  }

  private void initCurrentAlpha(float transformPos, int itemIndex)
  {
    ImageView currentImage = mActivity.findViewById(mItems.get(itemIndex));
    if (transformPos <= MINUS_INFINITY_EDGE || transformPos >= PLUS_INFINITY_EDGE)
      currentImage.setAlpha(ALPHA_TRANSPARENT);
    else if (transformPos == SETTLED_PAGE_POSITION)
      currentImage.setAlpha(ALPHA_OPAQUE);
    else
      currentImage.setAlpha(ALPHA_OPAQUE - Math.abs(transformPos));
  }

  private boolean isLeftEdge(int scrollX)
  {
    return scrollX == 0;
  }

  private boolean isRightEdge(int scrollX)
  {
    return scrollX == mPager.getWidth() * mAdapter.getCount();
  }

  private void restoreInitialAlphaValues()
  {
    for (int j = mItems.size() - 1; j >= 0; j--)
    {
      View view = mActivity.findViewById(mItems.get(j));
      view.setAlpha(ALPHA_OPAQUE);
    }
  }

  private void recalculateScrollDirection(float positionOffset, int positionOffsetPixels)
  {
    if (mShouldCalculateScrollDirection)
    {
      mIsScrollToRight =
          POSITION_OFFSET_BASE > positionOffset && positionOffsetPixels > POSITION_OFFSET_PIXELS_BASE;
      mShouldCalculateScrollDirection = false;
    }
  }

  private int getClientWidth()
  {
    return mPager.getMeasuredWidth() - mPager.getPaddingLeft() - mPager.getPaddingRight();
  }

  @NonNull
  private static String makePagerFragmentTag(int index, int pagerId)
  {
    return ANDROID_SWITCHER_TAG_SEGMENT + pagerId + SEPARATOR_TAG_SEGMENT + index;
  }
}
