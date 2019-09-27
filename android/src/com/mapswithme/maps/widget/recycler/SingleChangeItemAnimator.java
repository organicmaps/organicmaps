package com.mapswithme.maps.widget.recycler;

import androidx.annotation.Nullable;
import androidx.core.view.ViewCompat;
import androidx.core.view.ViewPropertyAnimatorCompat;
import androidx.core.view.ViewPropertyAnimatorListener;
import androidx.recyclerview.widget.RecyclerView;
import androidx.recyclerview.widget.SimpleItemAnimator;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.util.UiUtils;

public class SingleChangeItemAnimator extends SimpleItemAnimator
{
  private static final int DURATION = 350;

  @Nullable
  private ViewPropertyAnimatorCompat mAnimation;
  private boolean mFinished;

  @Override
  public long getChangeDuration()
  {
    return DURATION;
  }

  @Override
  public boolean animateChange(final RecyclerView.ViewHolder oldHolder,
                               final RecyclerView.ViewHolder newHolder, int fromLeft, int fromTop,
                               int toLeft, int toTop)
  {
    mAnimation = ViewCompat.animate(oldHolder.itemView);
    if (mAnimation == null)
      return false;

    mFinished = false;
    ViewGroup group = (ViewGroup) ((ViewGroup) oldHolder.itemView).getChildAt(0);
    for (int i = 0; i < group.getChildCount(); i++)
      UiUtils.hide(group.getChildAt(i));

    int from = oldHolder.itemView.getWidth();
    int target = newHolder.itemView.getWidth();
    oldHolder.itemView.setPivotX(0.0f);
    mAnimation
        .setDuration(getChangeDuration())
        .scaleX(((float) target / (float) from))
        .setListener(new ViewPropertyAnimatorListener()
        {
          @Override
          public void onAnimationStart(View view) {}
          @Override
          public void onAnimationCancel(View view) {}

          @Override
          public void onAnimationEnd(View view)
          {
            mFinished = true;
            mAnimation.setListener(null);
            UiUtils.hide(oldHolder.itemView, newHolder.itemView);
            dispatchChangeFinished(oldHolder, true);
            dispatchChangeFinished(newHolder, false);
            dispatchAnimationsFinished();
            onAnimationFinished();
          }
        })
        .start();
    return false;
  }

  public void onAnimationFinished() {}

  @Override
  public void endAnimation(RecyclerView.ViewHolder item)
  {
    if (mAnimation != null)
      mAnimation.cancel();
  }

  @Override
  public void endAnimations()
  {
    if (mAnimation != null)
      mAnimation.cancel();
  }

  @Override
  public boolean isRunning()
  {
    return mAnimation != null && !mFinished;
  }

  @Override
  public boolean animateRemove(final RecyclerView.ViewHolder holder)
  {
    return false;
  }

  @Override
  public boolean animateAdd(RecyclerView.ViewHolder holder)
  {
    return false;
  }

  @Override
  public boolean animateMove(RecyclerView.ViewHolder holder, int fromX, int fromY, int toX, int toY)
  {
    return false;
  }

  @Override
  public void runPendingAnimations() {}
}
