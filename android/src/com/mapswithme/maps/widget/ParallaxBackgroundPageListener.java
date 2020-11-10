package com.mapswithme.maps.widget;

import android.widget.ImageView;

import androidx.annotation.NonNull;
import androidx.viewpager.widget.ViewPager;
import com.mapswithme.maps.purchase.BookmarksAllSubscriptionPageData;

import java.util.Collections;
import java.util.List;

import static androidx.viewpager.widget.ViewPager.SCROLL_STATE_DRAGGING;
import static androidx.viewpager.widget.ViewPager.SCROLL_STATE_IDLE;
import static androidx.viewpager.widget.ViewPager.SCROLL_STATE_SETTLING;

public class ParallaxBackgroundPageListener implements ViewPager.OnPageChangeListener
{
  private static final float ALPHA_TRANSPARENT = 0;
  private static final float ALPHA_OPAQUE = 1;
  private static final float INVALID_OFFSET = -1f;
  @NonNull
  private final List<BookmarksAllSubscriptionPageData> mItems;
  @NonNull
  private final ImageView mFrontImageView;
  @NonNull
  private final ImageView mRearImageView;
  @NonNull
  private Direction mDirection = Direction.NONE;
  private int mState = SCROLL_STATE_IDLE;
  private int mCurrentPagePosition = 0;
  private float previousPositionOffset = INVALID_OFFSET;

  public ParallaxBackgroundPageListener(@NonNull List<BookmarksAllSubscriptionPageData> items,
                                        @NonNull ImageView frontImageView,
                                        @NonNull ImageView rearImageView)
  {
    mItems = Collections.unmodifiableList(items);
    mFrontImageView = frontImageView;
    mRearImageView = rearImageView;
    setInitialState();
  }

  @Override
  public void onPageScrollStateChanged(int state)
  {
    if (mState == SCROLL_STATE_IDLE && state == SCROLL_STATE_DRAGGING)
      resetDirectionMarker();
    if (mState == SCROLL_STATE_SETTLING && state == SCROLL_STATE_IDLE)
      setIdleState();
    mState = state;
  }

  @Override
  public void onPageSelected(int position)
  {
    mCurrentPagePosition = position;
  }

  @Override
  public void onPageScrolled(int position, float positionOffset, int positionOffsetPixels)
  {
    if (positionOffset != 0 && setupDirection(position, positionOffset))
      mFrontImageView.setAlpha(ALPHA_OPAQUE - positionOffset);
  }

  private void setInitialState()
  {
    mFrontImageView.setImageResource(mItems.get(0).getImageId());
    mRearImageView.setImageResource(mItems.get(1).getImageId());
  }

  private void prepareForwardDirection(int position)
  {
    mRearImageView.setImageResource(mItems.get(position + 1).getImageId());
  }

  private void prepareBackwardDirection(int position)
  {
    mRearImageView.setImageResource(mItems.get(mCurrentPagePosition).getImageId());
    mFrontImageView.setAlpha(ALPHA_TRANSPARENT);
    mFrontImageView.setImageResource(mItems.get(position).getImageId());
  }

  private void setIdleState()
  {
    mDirection = Direction.NONE;
    mFrontImageView.setImageResource(mItems.get(mCurrentPagePosition).getImageId());
    mFrontImageView.setAlpha(ALPHA_OPAQUE);
  }

  private void resetDirectionMarker()
  {
    previousPositionOffset = INVALID_OFFSET;
  }

  private boolean setupDirection(int position, float positionOffset)
  {
    if (previousPositionOffset == INVALID_OFFSET)
    {
      previousPositionOffset = positionOffset;
      return false;
    }
    Direction newDirection;
    if (previousPositionOffset < positionOffset && mCurrentPagePosition >= position)
      newDirection = Direction.FORWARD;
    else
      newDirection = Direction.BACKWARD;
    if (newDirection == mDirection)
      return true;
    switchDirection(newDirection, position);
    return true;
  }

  private void switchDirection(@NonNull Direction direction, int position)
  {
    switch (direction)
    {
      case NONE:
        break;
      case FORWARD:
        prepareForwardDirection(position);
        break;
      case BACKWARD:
        prepareBackwardDirection(position);
        break;
    }
    mDirection = direction;
  }

  private enum Direction
  {
    NONE, FORWARD, BACKWARD
  }
}
