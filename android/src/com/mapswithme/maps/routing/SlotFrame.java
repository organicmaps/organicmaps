package com.mapswithme.maps.routing;

import android.animation.Animator;
import android.content.Context;
import android.graphics.PointF;
import android.graphics.Rect;
import android.os.Build;
import android.support.annotation.IdRes;
import android.support.annotation.NonNull;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.util.ThemeUtils;
import com.mapswithme.util.UiUtils;

public class SlotFrame extends LinearLayout
{
  private static final int ANIM_SWAP = MwmApplication.get().getResources().getInteger(R.integer.anim_slots_swap);

  private Slot mSlotFrom;
  private Slot mSlotTo;

  private Slot mDraggedSlot;
  private Slot mNotDraggedSlot;
  private final PointF mDragStartPoint = new PointF();
  private boolean mSwapProgress;

  private int mTextColor;
  private int mHintColor;

  private class Slot
  {
    private final View mFrame;
    private final View mShadowedFrame;
    private final View mShadow;
    private final TextView mOrderText;
    private final TextView mText;
    private final View mDragHandle;
    private Rect mOverlapRect;
    private Rect mHitRect;

    private final int mOrder;
    private MapObject mMapObject;

    private final Animator.AnimatorListener mCancelAnimationListener = new UiUtils.SimpleAnimatorListener()
    {
      @Override
      public void onAnimationEnd(Animator animation)
      {
        cancelDrag();
        mFrame.animate().setListener(null);
        RoutingController.get().swapPoints();
      }
    };

    Slot(View frame, @IdRes int id, int order)
    {
      mFrame = frame.findViewById(id);
      mShadowedFrame = mFrame.findViewById(R.id.shadowed_frame);
      mShadow = mFrame.findViewById(R.id.shadow);
      mOrderText = (TextView) mFrame.findViewById(R.id.order);
      mText = (TextView) mFrame.findViewById(R.id.text);
      mDragHandle = mFrame.findViewById(R.id.drag_handle);

      mFrame.setOnClickListener(new View.OnClickListener()
      {
        @Override
        public void onClick(View v)
        {
          RoutingController.get().searchPoi(mOrder);
        }
      });

      mOrder = order;
      mOrderText.setText(String.valueOf(mOrder));

      setMapObject(null);
    }

    private void updateText()
    {
      if (mMapObject == null)
      {
        if (mOrder == 1)
          mText.setText(R.string.p2p_from);
        else
          mText.setText(R.string.p2p_to);

        mText.setTextColor(mHintColor);
        return;
      }

      if (mMapObject instanceof MapObject.MyPosition)
        mText.setText(R.string.p2p_your_location);
      else
        mText.setText(mMapObject.getName());

      mText.setTextColor(mTextColor);
    }

    void setMapObject(MapObject mapObject)
    {
      mMapObject = mapObject;
      if (mMapObject != null)
        mMapObject.setDefaultIfEmpty();

      updateText();
    }

    private void checkHitRect()
    {
      if (mHitRect == null)
      {
        mHitRect = new Rect();
        mFrame.getHitRect(mHitRect);
      }
    }

    boolean overlaps(Slot slot)
    {
      if (mOverlapRect == null)
      {
        mOverlapRect = new Rect();
        mFrame.getHitRect(mOverlapRect);

        int margin = UiUtils.dimen(R.dimen.routing_slot_overlap_margin);
        mOverlapRect.inset(margin, margin);
      }

      slot.checkHitRect();
      mOverlapRect.offset((int)mFrame.getTranslationX(), (int)mFrame.getTranslationY());
      boolean res = Rect.intersects(slot.mHitRect, mOverlapRect);
      mOverlapRect.offset((int)-mFrame.getTranslationX(), (int)-mFrame.getTranslationY());
      return res;
    }

    boolean handleHit(MotionEvent event)
    {
      checkHitRect();
      return (mHitRect.contains((int)event.getX(), (int)event.getY()) &&
              event.getX() > mDragHandle.getLeft() + mHitRect.left);
    }

    void setDragging(boolean dragging)
    {
      MarginLayoutParams lp = (MarginLayoutParams) mShadowedFrame.getLayoutParams();
      lp.topMargin = UiUtils.dimen(dragging ? R.dimen.routing_shadow_top_margin_dragging
                                            : R.dimen.routing_shadow_top_margin);
      lp.bottomMargin = UiUtils.dimen(dragging ? R.dimen.routing_shadow_bottom_margin_dragging
                                               : R.dimen.routing_shadow_bottom_margin);
      mShadowedFrame.setLayoutParams(lp);
      mShadow.setAlpha(dragging ? 0.6f : 1.0f);

      mShadowedFrame.setBackgroundResource(ThemeUtils.getResource(getContext(), dragging ? R.attr.routingSlotPressed
                                                                                         : R.attr.routingSlot));
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP)
        mFrame.setElevation(dragging ? 1.0f : 0.0f);
    }

    void moveViewTo(float x, float y)
    {
      if (getOrientation() == HORIZONTAL)
        mFrame.setTranslationX(x - mDragStartPoint.x);
      else
        mFrame.setTranslationY(y - mDragStartPoint.y);
    }

    void swapAnimated(Slot other)
    {
      mSwapProgress = true;

      MapObject point = other.mMapObject;
      other.setMapObject(mMapObject);
      setMapObject(point);

      setDragging(false);
      checkHitRect();
      other.checkHitRect();

      float offsetX = (other.mHitRect.left - mHitRect.left);
      float offsetY = (other.mHitRect.top - mHitRect.top);
      float otherOffsetX = (mFrame.getTranslationX() - offsetX);
      float otherOffsetY = (mFrame.getTranslationY() - offsetY);

      mFrame.setTranslationX(offsetX);
      mFrame.setTranslationY(offsetY);
      mFrame.animate()
            .setDuration(ANIM_SWAP)
            .translationX(0.0f)
            .translationY(0.0f)
            .setListener(mCancelAnimationListener)
            .start();

      other.mFrame.setTranslationX(otherOffsetX);
      other.mFrame.setTranslationY(otherOffsetY);
      other.mFrame.animate()
                  .setDuration(ANIM_SWAP)
                  .translationX(0.0f)
                  .translationY(0.0f)
                  .setListener(null)
                  .start();
    }
  }

  @Override
  protected void onFinishInflate()
  {
    super.onFinishInflate();

    mTextColor = ThemeUtils.getColor(getContext(), android.R.attr.textColorPrimary);
    mHintColor = ThemeUtils.getColor(getContext(), R.attr.secondary);

    setBaselineAligned(false);
    setClipChildren(false);
    setClipToPadding(false);
    setClickable(true);

    int padding = UiUtils.toPx(8);
    setPadding(padding, padding / 4, padding, padding);

    mSlotFrom = new Slot(this, R.id.from, 1);
    mSlotTo = new Slot(this, R.id.to, 2);
  }

  private boolean isDragging()
  {
    return (mDraggedSlot != null);
  }

  private void cancelDrag()
  {
    if (mDraggedSlot == null)
      return;

    mDraggedSlot.moveViewTo(mDragStartPoint.x, mDragStartPoint.y);

    mDraggedSlot.setDragging(false);
    mDraggedSlot = null;
    mNotDraggedSlot = null;
    mSwapProgress = false;
  }

  private void startDrag(Slot slotToDrag, MotionEvent event)
  {
    if (isDragging() || event.getAction() != MotionEvent.ACTION_DOWN)
      return;

    mDraggedSlot = slotToDrag;
    mDraggedSlot.setDragging(true);
    mNotDraggedSlot = (mDraggedSlot == mSlotFrom ? mSlotTo : mSlotFrom);
    mDragStartPoint.set(event.getX(), event.getY());
  }

  @Override
  public boolean onInterceptTouchEvent(MotionEvent event)
  {
    switch (event.getActionMasked())
    {
      case MotionEvent.ACTION_DOWN:
        if (mSlotFrom.handleHit(event))
        {
          startDrag(mSlotFrom, event);
          return true;
        }

        if (mSlotTo.handleHit(event))
        {
          startDrag(mSlotTo, event);
          return true;
        }
        break;
    }

    return false;
  }

  @Override
  public boolean onTouchEvent(@NonNull MotionEvent event)
  {
    if (mSwapProgress)
      return false;

    if (!isDragging())
      return super.onTouchEvent(event);

    switch (event.getActionMasked())
    {
      case MotionEvent.ACTION_MOVE:
        if (mDraggedSlot.overlaps(mNotDraggedSlot))
        {
          mDraggedSlot.swapAnimated(mNotDraggedSlot);
          break;
        }

        mDraggedSlot.moveViewTo(event.getX(), event.getY());
        break;

      case MotionEvent.ACTION_CANCEL:
      case MotionEvent.ACTION_UP:
        cancelDrag();
        break;
    }

    return true;
  }

  public SlotFrame(Context context)
  {
    super(context);
  }

  public SlotFrame(Context context, AttributeSet attrs)
  {
    super(context, attrs);
  }

  public SlotFrame(Context context, AttributeSet attrs, int defStyleAttr)
  {
    super(context, attrs, defStyleAttr);
  }

  public void update()
  {
    mSlotFrom.setMapObject(RoutingController.get().getStartPoint());
    mSlotTo.setMapObject(RoutingController.get().getEndPoint());
  }

  private void fadeSlot(Slot slot, boolean out)
  {
    slot.mFrame.setAlpha(out ? 1.0f : 0.1f);
    slot.mFrame.animate()
               .alpha(out ? 0.1f : 1.0f)
               .setDuration(RoutingPlanController.ANIM_TOGGLE)
               .start();
  }

  public void fadeSlots(boolean out)
  {
    fadeSlot(mSlotFrom, out);
    fadeSlot(mSlotTo, out);
  }

  public void unfadeSlots()
  {
    mSlotFrom.mFrame.clearAnimation();
    mSlotFrom.mFrame.setAlpha(1.0f);

    mSlotTo.mFrame.clearAnimation();
    mSlotTo.mFrame.setAlpha(1.0f);
  }
}
